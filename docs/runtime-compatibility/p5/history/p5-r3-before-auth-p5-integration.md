# P5-R3: integración y conciliación de aceptaciones x86

Rama `develop`, base `46e3ee05fc29ad090a1d6f7b1cb5e3dca9855350`. Esta actualización
regenera documentación y artefactos locales, sin commit, publicación ni despliegue.
**P5 sigue sin aceptación global**: las aprobaciones siguientes tienen candidato,
perfil y escenarios explícitos. La entrega actual es exclusivamente Linux i386.

## Evidencia conciliada

| Checkpoint | Binario SHA-256 | Alcance observado en SRCDS |
|---|---|---|
| Adaptador `auth014-r3` | `ddfdf9bf2f9bde625719ba171c59f7cac8b56cbe136a15e4911c7acbf00383ad` | Un evento real BeginAuthSession por 014, forwards públicos de validación, identidad, stats enteros, debug 12, cierre y restauración |
| Streaming y candidato P5-R3 | `bc76b00d0614f8f8489f8d3d30e3d654976f3bfde689f17d8731018d6c7ac639` | 32 completions, cuatro respuestas streaming y 24 bytes correctos, contexto completo, cambio de mapa, cancelación, cierre y restauración |

La [aceptación 014](auth014-srcds-acceptance.md) probó la autenticación con un
jugador en `competitive_dev`, LinuxGSM, instancia aislada en 27015. Los sources
del adaptador, resolver, hooks, debug y enlace allí registrados coinciden con los
actuales. Ese dato no convierte su prueba con jugador en una ejecución del
binario posterior: la autenticación y los stats positivos del candidato final
permanecen pendientes. No se inspeccionaron tickets ni se compararon sus bytes.

La [aceptación streaming](http-stream-srcds-acceptance.md) pertenece al binario
final. Pasa texto, binario con NUL, vacío, HTTP 503, timeout, rechazo TLS verificado,
conexión rechazada y contexto/payload de streaming. Dos peticiones streaming
quedaron encoladas antes del primer headers; la última tanda pasó tras cambiar a
`c1m1_hotel` con el mismo PID y módulo. No hubo errores SourceMod ni marcadores de
crash en esas fases. Ambos laboratorios restauraron el DEV habitual y sus hashes.

El fallo de contexto de P5-R2 queda **corregido y aceptado en el escenario actual**.
Ya no se enumera como prueba pendiente sin ejecutar. Tampoco quedan pendientes
el arranque, mapa y cierre explícito del binario `bc76…` en el perfil probado.
La carga tardía y el unload en caliente son pruebas diferentes y siguen abiertas.

## Validación integrada repetida

- Reconstrucción estricta L4D2 x86 con SDK 1.58a: SHA idéntico al binario aceptado
  para streaming. Los 53 headers y las dependencias de enlace coinciden con P0.
  `DT_NEEDED` usa `libtier0_srv.so` portable; sin módulos Steam/engine redistribuidos.
- Trece suites x86 pasan, incluida la comparación de API pública contra P0:
  47 natives, 11 forwards declarados, enums, typesets, registros y opcionalidad.
- SourceHook real: 18 comprobaciones de enlace 014 y control negativo del destino
  015. Streaming: 92 comprobaciones con fixture binaria independiente y control
  negativo de la lectura truncada del evento.
- CForward y VM aislada reales: 227 comprobaciones de marshalling y 16 ciclos de
  propiedad del entorno. Se reutilizó la VM x86 instrumentada de procedencia
  fijada; no se reconstruyó ni instaló otra VM. ASan/LSan/UBSan siguen activos,
  sin supresiones, también para lifecycle nativo y streaming.
- Doce pruebas de empaquetado/reversión pasan. El nuevo ZIP se extrae y valida;
  el roundtrip restaura bytes reales del baseline y elimina gamedata originalmente
  ausente en un sandbox, conservando una configuración ajena.
- Harness de ejemplo, AoC y Family Share recompilados sin warnings. Los imports
  de esos tres SMX y un Family Share precompilado corresponden a natives registrados.
  Esto no certifica todos sus comportamientos en SRCDS con el candidato final.
- Inventario renovado: 2675 archivos examinados, 24 con referencias. Cinco son
  fixtures/copias de harness; los 19 archivos de producto permanecen identificados.
  Persisten dos llamadas HTTP de Connect con tres argumentos donde se requieren cinco.
- actionlint, sintaxis shell, hashes de evidencia y `git diff --check` comprobados.
  CI remota no ejecutada. No se iniciaron servidores durante esta actualización.

La [matriz T01–T36](p5/validation-results.json) mantiene niveles ejecutados y
pendientes por caso; `prior_candidate_evidence` atribuye pruebas del R3 anterior.
[Logs actuales](p5/r3/integration.json), [inventario](p5/api-inventory.json) y
[consumidores](p5/consumers.json) acompañan los informes de runtime originales.
Los resultados x64 anteriores son historia; no se repitieron ni empaquetaron.

## Artefactos y reversión

Conjunto nuevo en `.build/p5-r3/review/`, con `manifest.json` y `SHA256SUMS`:

| ZIP | Contenido |
|---|---|
| `steamworks-p5-r3-linux-x86-candidate.zip` | Extensión ELF32 aceptada, include, licencia y documentación conciliada |
| `steamworks-p5-r3-competitive-dev-rollback.zip` | Binario/include del baseline renovado; ausencia de gamedata en el manifiesto |
| `steamworks-p5-r3-source-review.zip` | Fuentes propias, scripts, pruebas, parches y diff contra HEAD; sin SDK/deps |
| `steamworks-p5-r3-vm-diagnostics.zip` | Solo VM/receptores sintéticos x86, licencias y procedencia; fuera de `addons` |

La [reversión](p5-rollback.md) distingue tres resultados: captura renovada por
lectura, roundtrip del ZIP en filesystem y restauración real del selector aislado
ya ejecutada. No se afirma una instalación/restauración en sitio del ZIP nuevo.
No se empaquetan CFG privados, datos, tickets, SDK, bibliotecas Steam ni VMs en el
payload instalable. Los ZIP y checksums P5, P5-R2 y de ambas aceptaciones permanecen
intactos. El [snapshot P5-R2](p5/history/p5-r2-integration-original.md) y su
[matriz original](p5/history/p5-r2-validation-results.json) conservan su alcance.

## Reproducción local

Con dependencias fijadas y herramientas disponibles, seleccionar `-m32`,
`SOURCEMOD_DIR`, `MMSOURCE_DIR`, `AMTL_DIR`, `SPCOMP` y la VM x86 instrumentada.
Las suites VM requieren `VM_TEST_CXX=clang++`, `VM_TEST_FLAGS` con `-m32
-fsanitize=address -shared-libasan -fsized-deallocation -fno-pie -no-pie` y el
runtime ASan de esa versión. Mantener las opciones registradas de ASan/LSan.

```sh
python3 scripts/test-integration.py --with-auth-binding --with-auth-forward \
  --with-sourcepawn-environment --output .build/mi-integracion-x86
python3 scripts/stage-artifact.py . .build/p5-r3/build/package .build/mi-candidato
EXTS_ARTIFACT_DIR="$PWD/.build/mi-candidato" bash scripts/ci-validate-artifact.sh
python3 scripts/prepare-review-artifacts.py --candidate .build/mi-candidato \
  --baseline .build/p5-r3/baseline --output .build/mi-entrega --label mi-checkpoint
```

Cada salida debe ser nueva. La receta de los archivos complementarios se incluye
en el ZIP de fuentes, bajo `review/`. Nada de este flujo accede al servidor para
instalar ni reemplaza su VM. El baseline de reversión es una captura, no el destino.

## Pendientes vigentes

- Autenticación/identidad/stats positivos con el binario final; cobertura completa
  de stats, forwards y consumidores precompilados/recompilados. R3 aporta evidencia
  previa positiva, no una aprobación de todos los casos del binario final.
- GC y ejercicio completo de cada interfaz habilitada; negativos de integración
  todavía cubiertos únicamente con proveedores simulados.
- Reinicialización en el mismo proceso, carga tardía/unload en caliente, owner
  unload con petición pendiente y eventos de generaciones antiguas en host real.
- Desconexión controlada de Steam sin destruir el runtime y operación sostenida
  con medición de recursos/logs.
- Instalación del ZIP recién generado y reversión en sitio desde su ZIP con arranque
  posterior. La restauración del selector aislado ya pasó y no vuelve a marcarse
  como nunca ejecutada.
- Correcciones externas de las dos llamadas HTTP de Connect. Windows y otras
  parejas de módulos quedan fuera del soporte de esta entrega x86.
