# P5-R2: integración con T36 y la VM corregida

Este documento conserva el checkpoint local anterior a la intervención. La
[aceptación posterior en SRCDS](srcds-acceptance.md) registra las pruebas reales,
el fallo de contexto HTTP streaming y las puertas todavía abiertas. Los ZIP y la
matriz de este checkpoint permanecen como evidencia histórica, sin regenerarse.

Seguimiento: el [contexto HTTP streaming](http-stream-context.md) tiene causa
reproducida en el proveedor y corrección con pruebas deterministas x86. El nuevo
binario pasa después la [aceptación acotada en SRCDS](http-stream-srcds-acceptance.md),
sin producción. Esa evidencia no modifica este checkpoint ni aprueba P5 globalmente.

Fecha: 2026-11-14. Rama `develop`, base
`46e3ee05fc29ad090a1d6f7b1cb5e3dca9855350`, cambios sin commit, publicación ni
intervención sobre SRCDS. Resultado: candidato local actualizado; aceptación
operativa global pendiente. Este informe sustituye al [checkpoint inicial](p5-pre-t36-integration.md),
cuyos resultados y artefactos se conservan en `p5/history/` y `.build/p5/review/`.

## Conjunto integrado

P1–P4 mantienen sus correcciones de stats, identidad, detours, IPv4, interfaces
y propiedad de recursos. [T36](t36-auth-forward.md) conserva el registro interno
`Param_Array, Param_Cell, Param_Cell`, enviando bytes compactos con `PushArray`,
longitud original, AccountID completo, padding definido y sin copyback. Las
firmas, enums, callbacks, registros y opcionalidad del inventario público no cambian.

La [corrección de SourcePawn](sourcepawn-vm-asan.md) posee los motores mediante
sus tipos concretos y completa el cierre del entorno. Se aplican los parches a
copias de la revisión fijada, con cabeceras públicas idénticas y ASan/LSan activos.
Las VMs resultantes son herramientas aisladas de pruebas; el paquete instalable
contiene únicamente la extensión y su include como payload runtime. No reemplaza
la VM de SourceMod del servidor.

La [arquitectura](architecture.md), el [registro de compatibilidad](compatibility.md)
y la [reversión](p5-rollback.md) describen los contratos vigentes. El soporte del
perfil interno sigue limitado a la pareja Linux i386 identificada en P4.

## Validación repetida

- Build completo Linux x86 con warnings como errores, SDK 1.58a y dependencias
  fijadas del builder P0. Se contrastaron los 53 headers y librerías de enlace
  con sus hashes; no se actualizó el SDK.
- Reconstrucción de las VMs x86_64 y x86, seguida de 16 ciclos de creación/cierre
  y 227 comprobaciones de autenticación por arquitectura. ASan/LSan permanecen
  activos, incluida la liberación con tamaño y la detección de tipos incompatibles.
- Diez suites integradas pasan por arquitectura. GCC/UBSan conserva las suites
  nativas; Clang y el runtime ASan correspondiente ejercitan las dos suites de VM.
  Compilador, flags y tiempo medido por suite se registran. El test de prefijos
  x86 conserva su ejecución normal; no certifica memoria interna de Steam.
- ASan con detección de fugas sobre lifecycle nativo en host: sin errores observados.
- Once pruebas de empaquetado/reversión, incluidos nombres por checkpoint,
  rechazo de rutas inválidas, contenido inesperado, arquitectura y no sobrescritura.
- Harness, AoC y Family Share recompilados sin warnings con SourcePawn 1.12.0.7259.
  Sus imports y los de un Family Share precompilado corresponden a natives
  registrados. No se cargaron esos consumidores en SRCDS.
- Nuevo inventario público idéntico a P0. El corpus léxico tiene 2671 archivos
  examinados y 20 con referencias; el incremento frente a P0 es el fixture de
  autenticación de este repositorio. Los 19 archivos de producto y las dos
  discrepancias HTTP de Connect permanecen identificados.
- Paquete extraído validado contra el manifiesto, sin SDK/módulos Steam,
  configuración privada ni SMX de pruebas. Reversión probada desde los ZIP con
  archivos reales del candidato y del baseline en un directorio temporal.
- Workflow revisado con actionlint y `git diff --check` limpio. CI remota no ejecutada.

La extensión tiene SHA-256
`4830b71afa8d1a8b64e412071871165f849953667cc89499bd04612a1589687e`.
Es idéntica al build T36 y distinta del binario ejercitado en SRCDS en P4. Las
trazas P4 siguen probando ese checkpoint y el contrato del módulo identificado;
no se atribuyen al nuevo candidato pruebas de arranque, mapa o descarga en servidor.
T16 queda pendiente para el candidato actual, aunque exista evidencia histórica.

La matriz [T01–T36 y resultados actuales](p5/validation-results.json) distingue
alcances unitarios, integración aislada y ejecución real. T36 pasa en su alcance
local determinista con CForward y VM reales; no se probó autenticación Steam real.

## Artefactos nuevos

Directorio `.build/p5-r2/review/`, todos identificados en `manifest.json` y
`SHA256SUMS`:

| Archivo | Contenido y uso |
|---|---|
| `steamworks-p5-r2-linux-x86-candidate.zip` | Extensión, include, licencia y documentación actual; candidato para revisión |
| `steamworks-p5-r2-competitive-dev-rollback.zip` | Binario e include anteriores; ausencia de gamedata registrada en el manifiesto |
| `steamworks-p5-r2-source-review.zip` | Fuentes propias, tests, scripts, parches SourcePawn y diff contra HEAD; sin SDK/deps ni pedido privado |
| `steamworks-p5-r2-vm-diagnostics.zip` | VMs instrumentadas, receptores/harnesses sintéticos, licencias y procedencia; fuera de `addons`, sin instalación automática |

Los hashes de ZIP se mantienen en el manifiesto externo para evitar una referencia
circular dentro del propio paquete. Los hashes de extensión, VMs y fuentes están
además en los resultados del repositorio. Los binarios diagnósticos requieren el
runtime Clang registrado; no sustituyen librerías del servidor. Los ZIP iniciales
P5 y sus checksums permanecen intactos.

## Reproducir sin desplegar

Para reconstruir y validar una VM con fuentes y herramientas ya disponibles:

```bash
SOURCEMOD_DIR=/ruta/sourcemod SPCOMP=/ruta/runtime/scripting/spcomp64 \
  VM_TEST_BUILD_DIR="$PWD/.build/mi-vm-x64" \
  AMBUILD_PYTHON=/ruta/venv/bin/python AMBUILD_BIN=/ruta/venv/bin/ambuild \
  bash scripts/test-sourcepawn-vm-asan.sh
```

Repetir con `VM_TEST_ARCH=x86` y una salida nueva. Para la regresión completa,
seleccionar `SOURCEPAWN_VM`, `SPCOMP`, `SOURCEMOD_DIR`, `AMTL_DIR` y el runtime ASan
correspondiente. Usar `VM_TEST_CXX=clang++` y `VM_TEST_FLAGS` con instrumentación,
`-shared-libasan -fsized-deallocation -fno-pie -no-pie`; las suites nativas mantienen
su `CXX`/`TEST_CXXFLAGS`. Ejecutar:

```bash
python3 scripts/test-integration.py --with-auth-forward \
  --with-sourcepawn-environment --output .build/mi-integracion
python3 scripts/stage-artifact.py . .build/p5-r2/build/package .build/mi-candidato
EXTS_ARTIFACT_DIR="$PWD/.build/mi-candidato" bash scripts/ci-validate-artifact.sh
python3 scripts/prepare-review-artifacts.py --candidate .build/mi-candidato \
  --baseline .build/p5-r2/baseline --output .build/mi-entrega --label p5-r2
```

Cada salida debe ser nueva. El baseline es una captura previa de archivos, no una
ruta del servidor. La imagen y revisiones utilizadas están en los resultados;
los scripts generales de adquisición/CI siguen ramas y no reemplazan ese registro.

## Puertas abiertas

No se propone despliegue. Siguen pendientes las pruebas actuales I/R de interfaces,
HTTP/TLS/timeout, stats y GC remotos, carga de consumidores, desconexión sin
invalidar el runtime y operación sostenida. T16 y T35-I necesitan ejecutar el
candidato y una reversión con arranque real en una intervención autorizada.
Las llamadas HTTP de Connect requieren una corrección externa. Windows y otras
parejas de módulos están fuera del soporte declarado. Las comprobaciones locales
aprobadas no equivalen a aceptación operativa global.
