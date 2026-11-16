# Revisión 12: buffers de GC y helper textual HTTP

Se ejecuta la petición `/media/lechuga/OneDrive/steamworks/12-revision-develop-2026-11-16.md`.
La base inicial real es `a0016a23adc70dd9c1bdf2bf22bf8ae0e685dbc6`, exactamente la
revisada en ese documento. Ambos problemas persistían. Se corrigen únicamente
REV-GC-01 y REV-HTTP-02, con diffs separados y sin cambios de firmas, registros,
typesets, opcionalidad, SDK o interfaces 014/015.

**Estado actual:** correcciones locales y regresiones aprobadas, con
[aceptación SRCDS posterior de alcance limitado](review12-srcds-acceptance.md):
HTTP real y GC real vacío/local tipado pasan; recepción GC remota positiva y CI
propio siguen pendientes. El [registro local previo](review12/results.json)
conserva el estado anterior a esa aceptación. La matriz y los paquetes P5-R18
son evidencia del binario anterior, no de este worktree.
El documento original de OneDrive y las evidencias históricas no se modifican.

## REV-GC-01: capacidad respaldada, contenido válido

La implementación anterior entregaba `cubDest` bytes del destino del proveedor
al forward, incluso en errores. La regresión usa un destino de 4096 bytes con
relleno `0xa5` y tres bytes válidos `00 80 ff`: detecta el sobrante copiado por
el código anterior, sin emplear datos reales ni memoria del motor.

Se mantiene el argumento histórico `destinationLength` como capacidad. El forward
recibe una copia privada de esa capacidad, inicializada a cero. Solo se copia el
prefijo indicado por el tamaño en un éxito coherente. Esto distingue la capacidad
del contenido definido por [RetrieveMessage de Valve](https://partner.steamgames.com/doc/api/ISteamGameCoordinator#RetrieveMessage).

| Resultado/contexto del proveedor | Datos entregados al forward |
|---|---|
| OK, tamaño presente y coherente, destino válido si hay bytes | Tipo del proveedor si está presente, prefijo válido, resto a cero; `messageSize` es la cantidad copiada |
| Error, incluido NoMessage/BufferTooSmall | No se leen tipo/tamaño del proveedor; tipo y tamaño del forward a cero, copia de capacidad completa a cero |
| OK sin puntero de tamaño | No se presume contenido válido: tipo/tamaño a cero y copia a cero |
| Capacidad cero | Capacidad declarada cero y almacenamiento vacío respaldado por un byte cero |
| Tamaño mayor que capacidad o éxito no vacío sin destino | Se omite el forward, se registra el error y se conserva el resultado original |
| Capacidad no representable o fallo de reserva | Se omite el forward, se registra el error y se conserva el resultado original |

El límite de capacidad se deriva de `INT32_MAX` redondeado hacia abajo a celdas
Pawn de cuatro bytes: **2147483644**. Evita que el redondeo del almacenamiento del
forward supere un objeto de tamaño con signo. No es un umbral de política para
paquetes. La copia consume memoria proporcional a la capacidad y puede fallar;
la prueba de reserva fallida es controlada, no una prueba de agotamiento del SO.

Las defensas se aplican después de la llamada original; no se añaden garantías
para invocar el SDK fuera de sus precondiciones. Los casos nulos usan un doble
que permite comprobar esa ruta sin afirmar que Steam acepte todos esos punteros.

El destino y las salidas originales del proveedor no se limpian ni sobrescriben.
No hay copyback. El array es binario y puede carecer de NUL si el mensaje llena
la capacidad; los receptores deben usar `messageSize`, no tratarlo como texto.
El cero de cola no se anuncia como parte del mensaje.

Se preservan el único `SH_CALL`, la guarda de generación y la supercesión.
El forward sigue ejecutándose ante errores coherentes, permitiendo los overrides
históricos: el valor agregado cero conserva el del SDK; si al hook le llega
`-1`, lo convierte en OK, y otro valor no cero lo sustituye. La prueba posterior
con la VM real concilia ese detalle: `ET_Event` agrega a cero los retornos
negativos de consumidores, por lo que un receptor que retorna `-1` conserva
`NoMessage` en la ruta real. Un override no transforma el snapshot en el destino
original: el buffer del motor sigue intacto.

## REV-HTTP-02: lectura SDK completa y copia limitada

El doble anterior aceptaba cualquier longitud. Ahora `TestHTTP` exige el tamaño
completo, entrega contenido verificable y puede escribir parcialmente antes de
fallar. La regresión de diez bytes con capacidad cuatro falla con el helper
anterior: solicitaba tres bytes al SDK y el proveedor estricto los rechazaba.

El helper lee todos los bytes en almacenamiento temporal y solo después publica
el prefijo que cabe, su NUL y los resultados. Es la operación que exige
[GetHTTPResponseBodyData de Valve](https://partner.steamgames.com/doc/api/ISteamHTTP#GetHTTPResponseBodyData);
no se utiliza la API de streaming como sustitución.

- El cuerpo completo debe caber en el dominio de tamaños SourcePawn: como máximo
  **2147483647 bytes**. Un tamaño mayor genera un error nativo antes de reservar
  o leer. No se introduce un límite silencioso de 64 KiB o similar.
- La reserva usa `new (std::nothrow)` y RAII. Si falla, retorna false, sin lectura
  del cuerpo ni cambios de destino, `written` o `truncated`. Sigue existiendo el
  coste temporal de un cuerpo completo; no se acredita inmunidad al agotamiento
  del sistema ni se reserva un cuerpo real de varios GiB para probarlo.
- La consulta de tamaño o lectura fallida conserva destino y salidas, aunque el
  proveedor haya escrito un prefijo en su buffer temporal antes de devolver false.
- En éxito se copian `min(bodySize, length - 1)` bytes y un NUL; `written` cuenta
  bytes sin ese NUL y `truncated` indica pérdida por capacidad. Un destino de un
  byte contiene solo NUL. Cuerpo vacío: NUL, cero escrito, no truncado y sin reserva.
- Se mantienen las validaciones de capacidad/direcciones y la precedencia de
  disponibilidad/handle. La aridad antigua de tres argumentos se rechaza antes
  de acceder a las salidas ausentes de los argumentos cuarto/quinto.

Solo se cambian comentarios del include público. Los tres consumidores existentes
recompilados conservan imports/firmas; la forma antigua de tres argumentos sigue
siendo incompatible. No se modifican Connect, Family Share ni AoC.

## Regresiones y matriz ejecutada

Las pruebas incluyen las implementaciones de producción. SDK, transporte
SourceHook y memoria Pawn se simulan en los fixtures de estos dos hallazgos.
No son recepción de GC remoto, ejecución en SRCDS o la VM SourcePawn del servidor.
Las suites offline de CForward/VM y binding SourceHook preexistentes se repiten
por separado en la integración, sin atribuirles una recepción GC real.

| Caso solicitado | Resultado y cobertura |
|---|---|
| GC-01 | Pasa: tres bytes en capacidad 4096; cola privada a cero, relleno original intacto |
| GC-02 | Pasa: dieciséis bytes en dieciséis de capacidad; contenido íntegro |
| GC-03 | Pasa: NoMessage sin escribir salidas; además ASan envenena esas salidas y no detecta lecturas |
| GC-04 | Pasa: BufferTooSmall conserva resultado/salidas SDK; snapshot y metadatos del callback saneados |
| GC-05 | Pasa: nulos, falta de tamaño/tipo y capacidad cero con almacenamiento coherente; simulación defensiva |
| GC-06 | Pasa: éxito incoherente omite callback antes de copiar; no es una respuesta observada en Steam |
| GC-07 | Pasa: stop/start en la llamada original no despacha una generación antigua ni extrae dos veces |
| GC-08 | Pasa: default, force-OK y overrides de error conservan la convención de retorno |
| HTTP-01 | Pasa: diez bytes → capacidad cuatro; SDK recibe diez, Pawn recibe tres + NUL |
| HTTP-02 | Pasa: capacidad dieciséis; diez bytes, NUL y no truncado |
| HTTP-03 | Pasa: capacidad exacta once; último byte y canarios preservados |
| HTTP-04 | Pasa: capacidad uno; lectura SDK completa, cadena vacía terminada y truncada |
| HTTP-05 | Pasa: cuerpo vacío; NUL, cero escrito, no truncado |
| HTTP-06 | Pasa: consulta fallida y lectura que escribe prefijo antes de fallar; destino/salidas sin publicar |
| HTTP-07 | Pasa: rango no admisible y reserva controlada fallida; límites representables grandes llegan al allocador sin reserva real |
| HTTP-08 | Pasa: capacidades no positivas, direcciones inválidas y array de tres argumentos con protección ASan |

[GC anterior rechazado por la regresión](review12/negative-gc.log),
[HTTP anterior rechazado](review12/negative-http.log),
[repetición del control negativo](review12/negative-repeat.log),
[integración final](review12/integration.json) y
[checks enfocados ASan/UBSan](review12/asan-focused-final2.log).
Las regresiones fallan de forma intencional contra los archivos originales del
commit base, no por un fallo del compilador o del entorno. No se afirma que todos
los casos de las matrices fallaran antes de la corrección.

Resultados: 15 suites x86, 132 checks de hooks en UBSan y 133 con ASan/UBSan
(incluye la guarda de salidas sin escribir), más 33 checks específicos del helper
textual. Se repiten propiedad HTTP, streaming (92), convivencia init (142 + 21),
forwards/VM (227) y ciclos de propiedad de VM (16), sin supresiones. Los tres
consumidores y dieciocho harnesses SourcePawn compilan sin warnings; no se cargan.

## Reproducir y consultar el diff

[Diff GC](review12/gc-change.patch), [diff HTTP](review12/http-change.patch)
y [comentarios de contrato](review12/public-contract-comments.patch).
El diff HTTP incluye la nueva fuente de prueba. El índice Git permanece vacío.
Los compiladores, arquitectura, SDK y opciones exactas están en el registro y
[toolchain](review12/toolchain.txt). Se comprueban 55 entradas SDK contra P0.

Las recetas son las ejecutadas en este workspace; requieren SDK 1.58a, los builders
por digest registrados y la VM diagnóstica x86 fijada en `.build/p5-r2/vm-x86`.
Para los negativos, preparar las dos entradas originales en una copia de trabajo
que ya contenga las correcciones y las regresiones:

```sh
mkdir -p .build/review12/source-before
git show a0016a23adc70dd9c1bdf2bf22bf8ae0e685dbc6:extension/hooks/swgchooks.cpp > .build/review12/source-before/swgchooks.cpp
git show a0016a23adc70dd9c1bdf2bf22bf8ae0e685dbc6:extension/http/swhttprequest.cpp > .build/review12/source-before/swhttprequest.cpp

docker run --rm --network none -v "$PWD:/workspace" -w /workspace \
  sha256:8b88d2213cf0eaa488c64f73bd11d7827dfc9b10f9aefc0de714463c6fa2db3b \
  bash docs/runtime-compatibility/review12/commands/run-negative.sh

docker run --rm --network none -v "$PWD:/workspace" -w /workspace \
  sha256:ce871f47542914ddaa60dd5a94df7c7e85c83908c764a5eeea3a0e9a651cb368 \
  bash docs/runtime-compatibility/review12/commands/run-asan-focused.sh
```

[Receta de integración](review12/commands/run-integration-final.sh) requiere además
el compilador en `/compiler/spcomp64`, como en las validaciones P5 anteriores.
[Build optimizado](review12/commands/run-build.sh) no instala el ELF.
Los tests controlan la reserva fallida en el ejecutable de prueba; no hay un
switch de prueba ni un allocador alternativo en la extensión de producto.

## Estado al terminar la corrección local, antes de la aceptación SRCDS

| Campo del encargo | Resultado |
|---|---|
| Responsable | Codex |
| SHA inicial | `a0016a23adc70dd9c1bdf2bf22bf8ae0e685dbc6` |
| GC / HTTP | Diffs separados, correcciones locales |
| Fallos previos | Dos regresiones representativas reproducidas en x86 |
| Evidencia posterior | Ambas matrices deterministas pasan; integración y sanitizadores pasan |
| Artefacto | ELF32 optimizado `72664c658f6d28e78a975ec98c96879dc641f96725206f514f159ea17125fcb3`, sin instalar |
| CI remoto nuevo | No ejecutado; no se hace commit ni push |
| SRCDS nuevo | No ejecutado; el encargo excluye operar servidores |
| Dictamen | Hallazgos corregidos localmente; aceptación runtime del nuevo binario pendiente |

La aceptación anterior P5-R18 permanece ligada al ELF `839ff075…`. No se le
atribuyen estas correcciones ni se reescriben sus resultados. Una futura prueba
autorizada debe usar el nuevo ELF, incluir truncamiento HTTP no vacío y distinguir
la recepción GC real de una cola vacía o del despacho de argumentos sintéticos.
La disponibilidad de un escenario autorizado de mensajes GC sigue siendo una
condición para validar esa recepción real. No hay evidencia nueva de Windows.
Todos los paquetes históricos y el documento de OneDrive permanecen intactos.

## Continuación autorizada en SRCDS

La [aceptación posterior](review12-srcds-acceptance.md), realizada el 2026-11-16
con este mismo ELF, pasa 372 comprobaciones: HTTP real, GC real con cola vacía
y mensajes de proveedor local a través del hook y la VM reales. El DEV quedó
restaurado y conectado a Steam. La recepción positiva GC remota y CI propio
siguen pendientes. La tabla anterior y `review12/results.json` conservan el
estado histórico de la fase local; no se reescriben sus paquetes ni OneDrive.
