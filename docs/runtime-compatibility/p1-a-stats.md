# P1-A: salidas de stats conservadas ante fallo

Fecha: 2026-11-14. Rama: `develop`. Base:
`46e3ee05fc29ad090a1d6f7b1cb5e3dca9855350`. Cambios locales, sin commit.

## Cambio y contrato

Se corrigieron los cuatro lectores de
[ssnatives.cpp](../../extension/natives/ssnatives.cpp): `SteamWorks_GetStatCell`,
`SteamWorks_GetStatAuthIDCell`, `SteamWorks_GetStatFloat` y
`SteamWorks_GetStatAuthIDFloat`.

Antes, las variantes flotantes convertían y copiaban un temporal sin inicializar
incluso si `GetUserStat` devolvía false. Las variantes enteras entregaban
directamente la salida del plugin al proveedor, de modo que una escritura del
proveedor seguida de fallo podía modificarla.

Ahora se consulta con un temporal inicializado del tipo exacto (`int32` o
`float`) y se copia a SourcePawn solo después del éxito. En fallo se devuelve
false y se conserva la salida previa, incluso si el proveedor escribió un dato
antes de devolver false. El éxito con cero sigue siendo éxito.

Los cuatro lectores comprueban los resultados de `LocalToString` y
`LocalToPhysAddr`, propagan su código mediante `ThrowNativeErrorEx` y no ejecutan
`GetUserStat` si falla una conversión. Se mantiene la precedencia anterior: primero
disponibilidad de interfaz, luego cliente conectado cuando corresponde y por
último conversión de argumentos.

No se cambiaron firmas, enums, registro de natives, argumentos legacy de
identidad ni `RequestStats*`. H08 y la política de AppID siguen pendientes.
Se documentó el contrato en `docs/ISteamUserStats.md` sin modificar el include
público ni los consumidores.

La CI Linux ejecuta ahora la suite x86 con UBSan antes de compilar la extensión.
Ese paso nuevo se validó localmente con el comando equivalente; no se disparó
GitHub Actions ni se afirma un resultado remoto.

## Pruebas deterministas

La [suite](../../tests/stats_natives_test.cpp) incluye el archivo de producción y
ejecuta los natives desde su tabla real. Simula los servicios de Steam/SourceMod;
no es una implementación alternativa de los lectores. El doble de identidad
no evalúa sus semánticas.

| Caso por variante | Resultado comprobado |
| --- | --- |
| Proveedor falla y no escribe | Retorno false y centinela intacto |
| Proveedor escribe y después falla | Retorno false y centinela intacto |
| Éxito con valor negativo o cero | Retorno true y representación exacta copiada |
| Dirección de salida inválida | Error de conversión y ninguna consulta Steam |
| Dirección de nombre inválida | Error, sin resolver salida ni consultar Steam |
| Interfaz ausente | False, sin conversión, sin error nuevo ni escritura |
| Cliente inexistente o desconectado | Error antes de conversión/consulta; solo variantes cliente |

Se comprueban también canarios adyacentes, overload seleccionado y número de
consultas. Son 9 escenarios para cada variante cliente y 7 para cada AuthID:
32 por ejecución.

Se ejecutó la misma suite contra una copia temporal del `ssnatives.cpp` original
obtenida con `git show HEAD:extension/natives/ssnatives.cpp`, manteniendo intacto
el árbol de trabajo. Las cuatro variantes originales fallan de forma
determinista al escribir el proveedor y devolver false; las cuatro corregidas
pasan. No se depende del valor accidental de un float sin inicializar para
demostrar la regresión.

## Validación ejecutada y límites

| Prueba | Entorno / resultado |
| --- | --- |
| T02/U, T03/U y T28/U, solo lectores de stats | 32 escenarios aprobados con GCC 13.3.0, x86_64 y UBSan |
| Misma suite x86 | 32 escenarios aprobados con GCC 12.2.0, `-m32` y UBSan dentro del contenedor identificado en P0 |
| Regresión contra baseline | Fallo esperado de las 4 variantes, aisladas por native |
| Compilación con headers reales | `ssnatives.cpp` compila sin warnings como objeto ELF32 i386 con el SDK y dependencias identificados en P0 |
| Inventario público | Comparación con P0: firmas, enums, typesets, registros y lista opcional sin cambios |
| T02/I, T03/I/R y T28/I | Pendientes: no se seleccionó un servidor SourceMod/L4D2 para marshalling y stats reales |

La compilación con headers reales es una comprobación de la unidad modificada,
no un build completo. Se reprodujo el comando GCC registrado en P0 omitiendo
únicamente `-Wno-pointer-to-int-cast`, una opción de C que rompe C++ con
`-Werror`. El resto de advertencias conserva `-Werror`. No se modificó
`AMBuildScript`; la incompatibilidad del build completo sigue pendiente.

El resultado U valida la lógica nativa con dobles; no certifica la ABI ni la
disponibilidad de Steam. UBSan no sustituye un detector de toda lectura sin
inicializar: la garantía se apoya también en temporales inicializados y en no
leerlos cuando la consulta falla. No se declara aceptación operativa de H01.

## Reproducción y evidencia

Comando normal:

```bash
bash scripts/test-stats-natives.sh
```

Para x86 se usó el contenedor ya existente de P0, sin red, con el repo en solo
lectura salvo `.build/`, pasando `TEST_CXXFLAGS=-m32`. No se instalaron paquetes
en el host ni se cambiaron dependencias.

El [registro JSON](p1-a/validation-results.json) contiene revisiones, hashes,
comandos y estados por nivel. Los logs y objetos locales están en `.build/p1-a/`
y los ejecutables en `.build/tests-stats*`; no se empaquetan como producto.
La instantánea P0 se conserva sin regenerar para mantener la comparación.

## Riesgo y reversión

Un plugin que dependiera de una escritura tras un retorno false verá ahora su
valor previo. Ese comportamiento es la corrección acordada, no un cambio de
firma. La simulación no prueba por sí sola direcciones ni errores de una VM real.

La reversión consiste en retirar únicamente el diff de `ssnatives.cpp` de esta
entrega y sus pruebas/documentación, conservando los cambios previos. No hay
despliegue que revertir. No se hicieron commit, merge ni release.
