# P1-D: IP pública IPv4 y signo de char

Fecha: 2026-11-14. Rama: `develop`. Base:
`46e3ee05fc29ad090a1d6f7b1cb5e3dca9855350`, con P1-A/B/C y documentación locales
conservadas. Sin commit, release ni despliegue. H05 corregido y verificado por
unidad; aceptación en servidor pendiente.

## Contrato y cambio

Se conservan firmas, registro y orden de argumentos:

```sourcepawn
native bool SteamWorks_GetPublicIP(int ipaddr[4]);
native int SteamWorks_GetPublicIPCell();
```

Los dos natives de [gsnatives.cpp](../../extension/natives/gsnatives.cpp) obtienen
`SteamIPAddress_t` y comprueban primero `m_eType == k_ESteamIPTypeIPv4` y después
`IsSet()`. Solo tras esas comprobaciones acceden a `m_unIPv4`. La condición usa
cortocircuito: no llama a `IsSet` para otra familia. El SDK 1.58a declara el
valor IPv4 en orden del host y la dirección IPv6 como otro miembro de la unión.
P1-D no añade una API IPv6.

Antes, `IsSet()` podía aceptar una IPv6 y el código leía después el miembro
IPv4. Además, un `char` descendente controlaba el bucle de octetos; si era sin
signo, la condición de salida no funcionaba como se pretendía.

Ahora el array se escribe con un índice `int` ascendente de 0 a 3 y shifts
controlados de 24, 16, 8 y 0 bits. Cada celda recibe un octeto entre 0 y 255,
del más significativo al menos significativo: `0x01020304` produce `[1,2,3,4]`.
No depende del signo de `char` ni de reinterpretar bytes de la memoria del host.

La variante de una celda copia los cuatro bytes del entero IPv4 a `cell_t`
mediante `memcpy`, con un `static_assert` de tamaños. Conserva el patrón completo
sin depender de una conversión fuera de rango de unsigned a signed.
`0x80000000` conserva su bit alto y se representa como `-2147483648` en los
targets x86 probados; `0xffffffff` se representa como `-1`, no como error.

Sin interfaz, con dirección no establecida o con familia IPv6, el array devuelve
false sin modificar las cuatro salidas y la variante de celda devuelve 0.
Para una IPv4 válida, el array comprueba el resultado de `LocalToPhysAddr` antes
de escribir y propaga un error de conversión mediante `ThrowNativeErrorEx`.
La familia/disponibilidad se comprueba antes de convertir el destino.

La firma exige un destino de cuatro celdas. Esta entrega comprueba su dirección
base; no incorpora un comprobador general de la extensión de buffers en la VM.
Los canarios de las pruebas demuestran escritura de cuatro celdas en el buffer
controlado, no validez de cualquier rango que entregue un plugin arbitrario.

No se cambiaron metadata, licencias, grupos, identidad textual ni H09. Tampoco se
alteró la resolución de interfaces, el SDK o la arquitectura soportada.

## Pruebas deterministas

[public_ip_test.cpp](../../tests/public_ip_test.cpp) ejecuta los dos natives
desde la tabla real. Usa `SteamIPAddress_t`, familias y constructores del SDK
identificado en P0; el GameServer y la memoria SourceMod son dobles.
Las expectativas de octetos son literales, independientes del bucle de producción.

Se prueban quince entradas contra cada native: **30 escenarios** por binario.

| Entrada / propiedad | Resultado comprobado |
| --- | --- |
| `1.2.3.4`, loopback y bit bajo | Orden exacto de octetos y celda |
| IPv4 cero/no establecida | False/0, sin modificación del array |
| IPv6 loopback, prefijo no cero y no establecida | Rechazo; no interpretar los primeros bytes como IPv4 |
| `203.0.113.9`, `128.0.0.0` y todos los bits a uno | Octetos correctos y patrón completo de celda, incluyendo representación negativa |
| Ceros interiores | No truncar ni omitir octetos |
| Interfaz ausente | Sin consulta al proveedor, conversión ni escritura |
| Dirección de salida inválida con IPv4 válida | Error antes de escribir, centinelas intactos |
| Dirección de salida inválida con IPv6 o interfaz ausente | Rechazo previo a la conversión, sin error nuevo |

También se comprueban canarios antes/después de las cuatro celdas, número de
consultas, número de conversiones y que el native de una celda no use memoria
de salida. Se probaron las familias definidas por el SDK; no se fabricaron
valores inválidos de enum para eludir UBSan.

El runner compila cada caso con `-fsigned-char` y `-funsigned-char`, ejecutando
UBSan en ambos. Se omite únicamente la advertencia de parámetros no usados por
otros natives legacy del archivo incluido en el test; el resto de advertencias
se trata como error. No se silencia `-Wtype-limits`.

Contra una copia del código anterior a P1-D:

- Ambos natives fallan la expectativa de rechazo de una IPv6 con prefijo no cero.
- La compilación con `-funsigned-char` falla con `-Werror=type-limits` por la
  comparación siempre verdadera del contador anterior. No se necesita ejecutar
  un bucle potencialmente incorrecto para demostrar esta regresión.

Son fallos esperados de fixtures aislados; no se cargó código defectuoso en un
servidor.

## Resultados y límites

| Nivel / prueba | Resultado |
| --- | --- |
| T04/U | Familia, dirección no establecida y orden de octetos comprobados |
| T05/B/U | Suite compila y pasa con char firmado y sin signo en x86_64 y x86 |
| T28/U, solo IP | Error controlado al convertir dirección base; sin escritura en fallo |
| T30/U, solo IPv4/celda | Bit alto y representación firmada preservados |
| T24/B | Firmas, enums, callbacks y registros SourcePawn sin cambios |
| Regresión P1-A/B/C | 92 casos previos siguen pasando por arquitectura |
| Compilación con headers reales | `gsnatives.cpp` compila sin warnings como objeto ELF32 i386 |
| CI Linux | Nueva suite con ambos signos de char y UBSan; lint local, ejecución remota pendiente |
| T04/I y ABI/runtime | Pendientes de servidor L4D2 Linux x86 y módulos identificados |

Cada arquitectura ejecuta los 30 casos dos veces por signo de char, además de
las 92 regresiones previas: 152 ejecuciones de escenario por arquitectura.
La simulación no valida el retorno por valor de la estructura desde una vtable
Steam real ni demuestra que el servidor entregue IPv6 en este contexto.

La comprobación con headers reales conserva el procedimiento diagnóstico de
P1-A/B/C: omite solo la opción de C `-Wno-pointer-to-int-cast` del comando GCC
registrado, mantiene `-Werror` en el resto y no cambia AMBuild. No es un build
completo enlazado; su bloqueo registrado en P0 sigue pendiente.

## Reproducción, evidencia y reversión

```bash
bash scripts/test-public-ip.sh
bash scripts/test-stats-natives.sh
bash scripts/test-detours.sh
```

Para x86 se usó `TEST_CXXFLAGS=-m32` en el contenedor P0, sin red y con fuentes
en solo lectura salvo `.build/`. El runner acepta `CXX`, `TEST_BUILD_DIR` y
`STEAMWORKS_SDK_DIR` (por defecto `sdk_158a/`). No se instalaron paquetes nuevos.

El [registro JSON](p1-d/validation-results.json) identifica fuentes, entorno,
comandos, resultados y hashes. Los logs/objetos locales están en `.build/p1-d/`;
se mantienen las instantáneas anteriores como evidencia histórica.

La reversión retira solo el diff de IP en `gsnatives.cpp`, suite/runner, paso CI
y documentación de P1-D, conservando las fases anteriores. No hubo despliegue
que revertir. No se declara aceptación operativa de H05, Windows, H09 ni de
los puntos pendientes de ABI/ciclo de vida.
