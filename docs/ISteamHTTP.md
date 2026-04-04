# ISteamHTTP

## Resumen

La interfaz `ISteamHTTP` sí es utilizable en el runtime del servidor dedicado
de Left 4 Dead 2.


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
que garantice una cadena terminada en `\0`.

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

- lee hasta `length - 1`
- agrega siempre terminador nulo
- reporta cuántos caracteres escribió
- reporta si el contenido fue truncado
- simplifica el consumo desde SourcePawn

Pero tiene una limitación intencional:

- si el body es más grande que el buffer, el texto se trunca

Por eso:

- usar `SteamWorks_GetHTTPResponseBodyData(...)` sigue siendo lo correcto para payloads binarios
- usar `SteamWorks_GetHTTPResponseBodyString(...)` es la opción conveniente para texto o JSON

## Patrón recomendado

1. Obtener primero el tamaño real del body con `SteamWorks_GetHTTPResponseBodySize(...)`
2. Leer como máximo `buffer_size - 1`
3. Cerrar manualmente el string con `buffer[readLength] = '\0'`

Ejemplo conceptual:

```pawn
int size;
if (SteamWorks_GetHTTPResponseBodySize(request, size) && size > 0)
{
	char body[1024];
	int readLength = size;
	if (readLength > sizeof(body) - 1)
	{
		readLength = sizeof(body) - 1;
	}

	if (SteamWorks_GetHTTPResponseBodyData(request, body, readLength))
	{
		body[readLength] = '\0';
		PrintToServer("body=%s", body);
	}
}
```

## Implicancia práctica

Si un plugin obtiene:

- `failure=0`
- `success=1`
- `code=200`
- y además un `body size` mayor que `0`

pero imprime un body vacío, el problema más probable no es la request HTTP ni
la extensión, sino la ausencia de terminador nulo al tratar el contenido como
string.

Si el body esperado es textual, también puede usarse directamente:

```pawn
char body[2048];
int written;
bool truncated;
if (SteamWorks_GetHTTPResponseBodyString(request, body, sizeof(body), written, truncated))
{
	PrintToServer("written=%d truncated=%d body=%s", written, truncated, body);
}
```
