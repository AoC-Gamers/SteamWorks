# P5-R19: integración de buffers GC y texto HTTP

`develop`, base `a0016a23adc70dd9c1bdf2bf22bf8ae0e685dbc6`; sin commit, push ni
despliegue. R19 incorpora REV-GC-01/REV-HTTP-02 y la
[aceptación SRCDS x86 de la revisión 12](review12-srcds-acceptance.md).
**GC remoto positivo permanece pendiente. No se declara aceptación global nueva.**
La API pública conserva firmas, tipos, registros y opcionalidad frente a P0.

## Qué acredita el candidato actual

El ELF optimizado es
`72664c658f6d28e78a975ec98c96879dc641f96725206f514f159ea17125fcb3`, idéntico al
probado en `competitive_dev` con LinuxGSM, árbol aislado y puerto 27015. El build
repetido fija `SOURCE_DATE_EPOCH=1794700800`; su fecha de ejecución se registra
por separado. Las fuentes y el include coinciden con la aceptación de revisión 12.

- HTTP: proveedor Steam real, 36 combinaciones cuerpo/capacidad, seis completions,
  contexto correcto, NUL, conteos, truncamiento y canarios. Seis intentos antes
  del envío conservan todas las salidas al fallar.
- GC: 80 consultas reales con cola vacía y diez casos de proveedor local tipado,
  a través de SourceHook, snapshot del candidato y VM reales. Mensaje corto con
  capacidad 4096, binario completo, vacío, NoMessage y BufferTooSmall pasan.
  La interfaz real queda restaurada después de cada caso local.
- Son 372 checks sin fallos, mismo PID antes/después de nuevo juego y quit limpio.
  DEV original restaurado, conectado a Steam y cuatro hashes protegidos intactos.

El proveedor local **no demuestra recepción desde el backend GC**. Se necesita
un escenario autorizado con mensaje remoto positivo para cerrar ese pendiente.
La [matriz conciliada](p5/validation-results.json) separa esa deuda, el alcance
actual y los 33 casos históricos/locales aprobados y tres no aplicables. Cada
caso conserva su procedencia; no se transfieren automáticamente los ensayos de
`839ff075…` al nuevo ELF. Auth/stats humanos, multi-consumidores, carga prolongada,
recuperación de red y demás ramas históricas no se ejecutaron otra vez en R19.

## Validación integrada repetida en la fase de empaquetado

- Quince suites x86, comparación completa de API con P0, controles de aridad,
  macro Windows activa, lifecycle, binding 014, streaming y VM pasan.
- ASan/LSan/UBSan nativos y VM offline, sin supresiones: 133 checks del hook GC,
  33 del lector HTTP estricto, 227 de forwards y 16 ciclos de propiedad de VM.
  La VM diagnóstica no se instala en SRCDS.
- Build `-O3 -DNDEBUG -m32 -Werror` reproduce el ELF aceptado; se recompilan tres
  consumidores, 19 harnesses, cuatro fixtures Connect, dos controles de include
  y los helpers ELF32 forward/capacidades/buffers, sin warnings nuevos.
- 55 entradas SDK y el fixture de 733 claves stats se conservan. Los 66 tests
  Python y el build de Connect pasan; sus salidas se escriben en el scratch de
  SteamWorks. No hay cambios de fuentes en Connect ni nueva aceptación de ese
  consumidor en SRCDS. Su parche completo contra `c695bf0…` sigue en source-review.
- Los ZIP candidato/fuentes/diagnósticos se extraen y verifican. Se repiten build
  y 15 suites desde las fuentes extraídas sin `.git`, y el replay de diagnósticos.
  Se aplica el parche Connect completo a su base y se contrastan los archivos.
- La captura de reversión se renueva por lectura; roundtrip de los ZIP reales en
  filesystem desechable conserva bytes originales, gamedata ausente y CFG ajenos.
  DEV se observa antes/después con mismo PID, selector y cuatro hashes protegidos.
  **La fase de empaquetado no inició, detuvo ni modificó servidores.**

[Resultados locales](p5/r19/integration.json),
[fuentes extraídas](p5/r19/source-extracted-integration.json),
[sanitizadores](p5/r19/asan-focused.log),
[consumidores](p5/r19/consumer-compilations.json),
[captura baseline](p5/r19/baseline-observation.json) y
[roundtrip de reversión](p5/r19/rollback-roundtrip.json).
La validación de buffers citada pertenece al ensayo anterior de revisión 12;
en el empaquetado se incorporó por igualdad exacta del payload. El seguimiento
T26/T35 posterior, descrito al final, sí instala y revierte el ZIP local exacto.

## Artefactos y reversión

Conjunto `.build/p5-r19/review/`, con manifiesto, verificación y `SHA256SUMS`:

| ZIP | Contenido |
|---|---|
| `steamworks-p5-r19-linux-x86-candidate.zip` | Solo extensión/include en addons; licencia y documentación conciliada |
| `steamworks-p5-r19-competitive-dev-rollback.zip` | Extensión/include habituales, captura por lectura, gamedata ausente |
| `steamworks-p5-r19-source-review.zip` | Fuentes, tests, evidencia, recetas offline, atributos Git y parche Connect completo |
| `steamworks-p5-r19-vm-diagnostics.zip` | VM, receptores y helpers de laboratorio fuera de addons; licencia/procedencia |

No se distribuyen SDK, bibliotecas del juego/Steam, CFG privadas, secretos ni
identidades. Los SMX HTTP de tres argumentos siguen requiriendo recompilación.
La [reversión](p5-rollback.md) produce el ZIP `55153d55…`, idéntico al aplicado en
R9. El baseline mantiene su SIGSEGV conocido al salir; no se acepta cierre limpio.
El empaquetado no aplicó el ZIP en un servidor; el seguimiento posterior sí lo
aplica en un árbol aislado. Los paquetes históricos permanecen intactos.

## Reproducción offline y límites

Extraer source-review y vm-diagnostics en directorios nuevos. Preparar SDK 1.58a
externo y compilador SourcePawn; verificar sus hashes y las imágenes fijadas.
El ZIP no contiene esas dependencias ni los includes externos de consumidores.
Desde la raíz de fuentes extraídas:

```sh
export REVIEW_SDK_DIR=/ruta/al/sdk_158a
export REVIEW_COMPILER_DIR=/ruta/a/sourcemod/scripting
export REVIEW_DIAGNOSTICS_DIR=/ruta/a/diagnosticos-extraidos

docker run --rm --network none -v "$PWD:/workspace" \
  -v "$REVIEW_SDK_DIR:/workspace/sdk_158a:ro" -w /workspace \
  sha256:8b88d2213cf0eaa488c64f73bd11d7827dfc9b10f9aefc0de714463c6fa2db3b \
  bash review/run-reproducible-build.sh

docker run --rm --network none -v "$PWD:/workspace" \
  -v "$REVIEW_SDK_DIR:/workspace/sdk_158a:ro" \
  -v "$REVIEW_COMPILER_DIR:/compiler:ro" \
  -v "$REVIEW_DIAGNOSTICS_DIR:/diagnostics:ro" \
  -e SOURCEPAWN_VM=/diagnostics/diagnostics/linux-x86/libsourcepawn.so \
  -w /workspace \
  sha256:ce871f47542914ddaa60dd5a94df7c7e85c83908c764a5eeea3a0e9a651cb368 \
  bash review/run-integration.sh
```

El [checkpoint anterior](p5/history/p5-r18-before-review12-integration-validation-results.json)
conserva la matriz de su propio binario. No se infieren Windows/x64 runtime,
proveedores desconocidos, todos los métodos SDK, ausencia global de fugas/OOM
ni recepción GC remota. Este registro de empaquetado era previo al commit
`adffed0` y a su CI exitoso; el seguimiento posterior consigna esa evidencia.
No se dispara CI ni se publica en este seguimiento sin commit.

## Revisión final de R19

La [revisión final](p5/r19/final-review/results.json) corrige el ejemplo HTTP raw:
la API requiere leer el tamaño completo, no un prefijo reducido. El ejemplo
textual usa el helper de cinco argumentos y los conteos se documentan en bytes.
También distingue las recetas build/integración portables del finalizador de
paquetes, ligado al checkout y las capturas privadas del workspace original.
Los ejemplos se compilan sin warnings y los paquetes finales se contrastan con
el árbol actual y la reversión archivada.

No cambia código nativo, include público ni fuente de los tests; se conservan
por hashes sus resultados x86 y el ELF aceptado de revisión 12. Los ZIP R19
anteriores a esta revisión se guardan en `.build/p5-r19/pre-final-review-request/`.
GC remoto positivo sigue pendiente, sin aceptación global nueva, commit ni
despliegue. No se opera el servidor.

## Seguimiento T26/T35 del ZIP local

La [aceptación posterior](p5-r19-rollback-srcds-acceptance.md) instala el ZIP
candidato exacto `b8a4a8cd…` y revierte con `55153d55…` en el mismo árbol detenido
de `competitive_dev`, mediante LinuxGSM y puerto 27015. Candidato y baseline
arrancan, conectan a Steam y pasan HTTP/cuerpo/contexto/callback/cierre con un
observador común. Ambos salen sin marcadores de crash en este perfil mínimo;
el SIGSEGV histórico del baseline no se reproduce ni se declara resuelto globalmente.
DEV queda restaurado, Steam conectado, cuatro hashes y 1843 CFG intactos.

Solo T26/T35 reciben esta aceptación directa del ZIP; no la pila completa de
consumidores ni los assets de `channel/develop`. GC remoto sigue pendiente.
El CI existente [43889902957](https://github.com/AoC-Gamers/SteamWorks/actions/runs/43889902957)
pasó siete jobs sobre `adffed0`; no se repite ni sustituye el ensayo runtime.
Los ZIP R19 permanecen intactos y la nueva evidencia se entrega por separado,
sin commit, push o producción.
