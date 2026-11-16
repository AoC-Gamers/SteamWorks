# ISteamHTTP

## Resumen

La interfaz `ISteamHTTP` tiene evidencia histórica de uso en el servidor dedicado
de Left 4 Dead 2. La implementación actual tiene pruebas deterministas de frontera,
propiedad y callbacks. La [aceptación P5-R2](runtime-compatibility/srcds-acceptance.md)
validó HTTP/TLS/timeout y detectó pérdida de contexto en streaming. La
[corrección x86 del contexto](runtime-compatibility/http-stream-context.md)
reproduce la causa en el proveedor real y pasa pruebas deterministas. La
[aceptación posterior en SRCDS x86](runtime-compatibility/http-stream-srcds-acceptance.md)
valida contexto y payload de streaming en cuatro tandas, incluida una tras cambio
de mapa, con cero fallos. Su alcance se limita al binario/perfil documentado.


## Uso esperado

La interfaz resulta adecuada para:

- checks simples de conectividad HTTP/HTTPS
- requests GET o POST contra servicios externos
- integración con APIs o microservicios auxiliares
- validaciones técnicas del entorno Steam en runtime

## Detalle importante sobre el body

El native:

- `SteamWorks_GetHTTPResponseBodyData(...)`

debe tratarse como una lectura de bytes del cuerpo HTTP, no como una función
que garantice una cadena terminada en `\0`. El tamaño de lectura debe ser el
cuerpo completo devuelto por `SteamWorks_GetHTTPResponseBodySize`; esta API
no permite solicitar únicamente un prefijo.

En la práctica esto significa que:

- el body puede haberse leído correctamente
- `SteamWorks_GetHTTPResponseBodySize(...)` puede devolver un tamaño válido
- pero si el plugin imprime el buffer directamente como string sin cerrar
  manualmente el terminador nulo, el resultado puede verse vacío o corrupto

## Helper textual adicional

Para casos donde el body se espera como texto, por ejemplo:

- JSON
- HTML
- texto plano

la extensión expone además:

- `SteamWorks_GetHTTPResponseBodyString(...)`

Este helper:

- lee el cuerpo completo mediante el SDK y copia hasta `length - 1` al destino
- agrega terminador nulo cuando devuelve éxito
- reporta cuántos bytes copió, excluido el NUL final
- reporta si el contenido fue truncado
- simplifica el consumo desde SourcePawn

Pero tiene una limitación intencional:

- si el body es más grande que el buffer, el texto se trunca

Por eso:

- usar `SteamWorks_GetHTTPResponseBodyData(...)` sigue siendo lo correcto para payloads binarios
- usar `SteamWorks_GetHTTPResponseBodyString(...)` es la opción conveniente para texto o JSON

Su firma requiere los cinco argumentos, incluidos `written` y `truncated`.
Las llamadas antiguas de tres argumentos se rechazan antes de acceder a salidas
ausentes. `written` excluye el terminador; no comparte el conteo histórico de
`GetClientSteamID`. Un fallo no garantiza contenido textual utilizable.

Cada request pertenece al plugin y a la generación del runtime que lo creó.
Shutdown o descarga del propietario lo invalidan; debe crearse uno nuevo tras
reinicialización. Cerrar el handle desde un callback es admisible: su destrucción
se difiere hasta salir del despacho. Véase [P3](runtime-compatibility/p3-resource-lifecycle.md).

## Patrón recomendado para texto

Usar el helper de cinco argumentos cuando el destino tenga capacidad limitada.
Lee el cuerpo completo mediante el SDK y solo después copia el prefijo que cabe:

```pawn
char body[2048];
int written;
bool truncated;
if (SteamWorks_GetHTTPResponseBodyString(request, body, sizeof(body), written, truncated))
{
	PrintToServer("written=%d truncated=%d body=%s", written, truncated, body);
}
```

Para lectura raw, consultar primero el tamaño y pasar **ese tamaño exacto**.
Si el cuerpo no cabe completo, ampliar el destino o usar el helper textual;
reducir `length` en `GetHTTPResponseBodyData` no produce un truncamiento válido.
Si se imprimen los bytes como texto, reservar además un byte para NUL:

```pawn
int size;
char body[1024];
if (SteamWorks_GetHTTPResponseBodySize(request, size) && size >= 0 && size < sizeof(body))
{
	if (size == 0 || SteamWorks_GetHTTPResponseBodyData(request, body, size))
	{
		body[size] = '\0';
		PrintToServer("body=%s", body);
	}
}
```

Este segundo ejemplo solo procesa cuerpos que caben íntegros; no trunca. Para
payloads binarios, conservar el tamaño y no interpretar el buffer como cadena.
Un estado HTTP 200 no acredita que la lectura del cuerpo haya tenido éxito:
comprobar su retorno, longitud y espacio para NUL antes de imprimir.

## Corrección posterior a P5-R18

La [revisión 12](runtime-compatibility/review12-buffers.md) añade lectura completa
temporal antes de truncar. Fallo del SDK o de reserva conserva destino y salidas.
El límite completo es el rango con signo de tamaños SourcePawn; excederlo genera
un error nativo antes de reservar. Requiere respuesta completada no streaming.
Además de las pruebas deterministas x86, la [aceptación SRCDS](runtime-compatibility/review12-srcds-acceptance.md)
valida cuerpos completos, truncados y vacíos con Steam HTTP real. Fallos extremos
de reserva/lectura siguen probados mediante dobles locales, sin forzar OOM en SRCDS.
