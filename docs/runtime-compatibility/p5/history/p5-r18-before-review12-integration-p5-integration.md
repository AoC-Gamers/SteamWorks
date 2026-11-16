# P5-R18: integración de las correcciones de CI de Windows

> Este informe fija el checkpoint P5-R18 anterior. El worktree contiene las
> [correcciones nativas de la revisión 12](review12-buffers.md), con un ELF distinto
> y [aceptación SRCDS separada](review12-srcds-acceptance.md) de HTTP real y GC
> real vacío/local tipado; recepción GC remota positiva pendiente. No se incorporan
> retrospectivamente estos resultados ni cambios a los paquetes P5-R18.

Rama `develop`; base de producto `cc523056f958a0a904b31dde90290b2364760c97`.
Esta actualización de documentación y paquetes permanece sin commit ni push.
La API pública se compara con P0 (`46e3ee0…`) y se conserva sin cambios.
[Matriz T01–T36](p5/validation-results.json): 33 casos aceptados y T13/T14/T34
no aplicables al perfil Linux L4D2 x86. No quedan casos pendientes en ese alcance.

## Correcciones incorporadas y evidencia remota

El primer CI de R17 detectó la colisión de la macro Windows `max` con cuatro
llamadas a `std::numeric_limits::max()`. La forma protegida por paréntesis compila
sin desactivar la macro ni las comprobaciones. El control negativo anterior y
el nuevo control x86 con la macro activa se conservan.

El siguiente CI confirmó esa compilación y encontró ocho imports de ConVar sin
resolver. El enlace Windows ahora declara `tier0.lib` junto a `tier1.lib`;
los símbolos se contrastaron con la librería SDK sin redistribuirla.

[CI SteamWorks 43861680010](https://github.com/AoC-Gamers/SteamWorks/actions/runs/43861680010)
pasó Linux, Windows, empaquetado y publicación sobre `cc52305`.
[CI Connect 43860123701](https://github.com/AoC-Gamers/Connect-Sourcepawn/actions/runs/43860123701)
pasó sobre `5f62f3c`. R18 conserva [ambas capturas](p5/r18/SteamWorks-CI.json),
con [el resultado de Connect](p5/r18/Connect-CI.json). No se dispara CI ni se
publica en esta actualización. Compilar Windows no acepta un perfil runtime
Windows: la aceptación operativa continúa limitada a Linux x86.

## Validación local repetida

- Quince suites x86 pasan, incluida comparación integral de API con P0 y
  control de la macro Windows activa. ASan/LSan/UBSan nativos y VM offline
  sin supresiones; reinicio, convivencia init, streaming y propiedad comprobados.
- Build optimizado `-O3 -DNDEBUG -m32 -Werror`, fecha del compilador fijada con
  `SOURCE_DATE_EPOCH=1794700800`. Reproduce el ELF aceptado
  `839ff0751b04141031bdc6dffc48662d0804f52227f2c91015bd87c6b84cf192`.
  El include conserva `a3e5a7a220dd4d7b46dab3226b41c68f2eb24c4ddf4943f0b943f87566b4f2d5`.
  La fecha fijada es la del binario reproducido; la captura registra la fecha
  real de esta validación. No se repite el ensayo con fecha libre de R17.
- Dieciocho harnesses runtime, tres consumidores, cuatro fixtures Connect y
  dos controles con includes antiguo/actual compilan sin warnings. La llamada
  HTTP antigua de tres argumentos se rechaza con error 034 esperado.
- Las 66 pruebas Python y el build de Connect pasan. Sus fuentes conservan los
  hashes del SMX aceptado; los 104 checks runtime pertenecen al ensayo R16 y
  no se presentan como ejecutados de nuevo en R18. Inventario sin aridades erróneas.
- Se verifican 152 fuentes, 55 entradas SDK, imports SMX, fixture de 733 claves
  stats y ambos helpers ELF32 de T11/T23. La VM diagnóstica conserva su hash y
  procedencia; no se instala en un servidor.
- Se valida el árbol candidato y su ZIP extraído; se reproduce build y quince
  suites desde el ZIP de fuentes sin `.git`. Los diagnósticos extraídos repiten
  227 checks de forward y 16 ciclos de propiedad de VM. Se aplica el parche
  completo de Connect a una extracción de su base para verificar el resultado.
- Captura baseline renovada por lectura, reversión desde ZIP en filesystem
  desechable, configuración ajena preservada y ausencia de gamedata restaurada.
  DEV se contrasta antes/después con mismo PID, selector, módulo y cuatro hashes.

[Logs R18](p5/r18/integration.json), [reproducción desde fuentes](p5/r18/source-extracted-integration.json),
[reversión archivada](p5/r18/rollback-roundtrip.json), [SDK y harnesses](p5/r18/sdk-harness-checks.json)
y [captura de reversión](p5/r18/baseline-observation.json). No se arranca, reinicia
ni modifica ningún servidor en esta actualización.

## Alcance conservado por hash

[T25/T29](last-two-srcds-progress.md) conserva préstamo real, perfil Web API del
propietario desconectado, grupo autorizado y entrada pendiente→autorizada del
mismo cliente. Las alternativas defensivas de identidad se cubren con pruebas
deterministas e invariante binaria del proveedor, sin forjar cuentas reales.

[T32](connection-srcds-acceptance.md) conserva un ciclo real de recuperación Steam,
81 checks y HTTP/interfaces estables. [T33](soak-srcds-acceptance.md) conserva veinte
minutos de carga local, con ventanas de reposo/recuperación y límites medidos.
Se mantienen autenticación/stats optimizados, T11 capacidades/GC local, T15 buffers,
T16 recarga quiescente, T20 propietario HTTP, T23 forwards y T26/T35 instalación y
reversión en sus alcances originales. ELF/include idénticos permiten atribuir
estos resultados al payload R18; el ZIP R18 nuevo no se instala.

[La matriz R17 anterior](p5/history/p5-r17-with-Windows-CI-fixes-validation-results.json),
las evidencias y todos los ZIP previos permanecen intactos. Los estados pendientes
de las capturas históricas de CI quedan conciliados con los éxitos remotos citados.

## Artefactos nuevos

Conjunto `.build/p5-r18/review/`, con manifiesto, verificación y `SHA256SUMS`:

| Archivo | Contenido |
|---|---|
| `steamworks-p5-r18-linux-x86-candidate.zip` | Extensión/include en addons, licencia, mapa y documentación actualizada |
| `steamworks-p5-r18-competitive-dev-rollback.zip` | Extensión/include originales; ausencia de gamedata registrada |
| `steamworks-p5-r18-source-review.zip` | Fuentes actuales con ambas correcciones Windows, atributos de bytes auditados, tests, evidencia y recetas offline; parche completo de Connect |
| `steamworks-p5-r18-vm-diagnostics.zip` | VM offline x86 y receptores sintéticos con licencia/procedencia, fuera de addons |

El ZIP de fuentes incorpora `.gitattributes`, necesario para conservar los bytes
que sustentan hashes de evidencia y wrappers SDK. Incluye el parche **completo**
de Connect `c695bf0…`→`5f62f3c…`, no un diff vacío contra su HEAD ya corregido.
Los helpers de Connect se incluyen para lectura; no hay SMX Connect en el payload
de SteamWorks ni se distribuyen sus proveedores/includes externos. Ese consumidor
permanece committed sin cambios nuevos; R18 solo actualiza SteamWorks.

La [reversión](p5-rollback.md) conserva exactamente el ZIP `55153d55…` ejercitado
en R9. El baseline mantiene su SIGSEGV conocido al salir; no se acepta su cierre
limpio. No se distribuyen CFG, claves, datos de jugadores ni SDK/bibliotecas de juego.

## Límites vigentes

No se certifican Windows/x64 runtime, otras versiones/proveedores, todos los
métodos SDK, GC/Core remoto, partidas completas ni capacidad durante varias horas.
T16 es serial/quiescente; T20 y T33 usan ventanas medidas, sin prueba universal
de ausencia de fugas SDK. Los consumidores HTTP antiguos de tres argumentos
requieren recompilación; el rollback de SteamWorks no sustituye la captura propia
del SMX/config de Connect antes de una eventual intervención autorizada.
Los metadatos de CI corresponden a las bases de producto ya publicadas; los
paquetes y documentos R18 no se publican ni se despliegan.

## Revisión final de R18

La [revisión final](p5/r18/final-review/results.json) concilia tres descripciones
auxiliares de CI con sus pasos realmente exitosos en `cc52305` y atribuye la
comparación de fecha libre a R17. No altera código nativo ni evidencias históricas.
Se revisan el diff, la matriz y los paquetes reales: hashes, límite de payload,
parche completo de Connect y reversión temporal con archivos ajenos preservados.
Los primeros ZIP R18 se conservan en `.build/p5-r18/pre-final-review/`; los
paquetes corregidos permanecen en `.build/p5-r18/review/`. La revisión no acepta
Windows runtime ni repite la reversión en SRCDS. Sin commit, push o despliegue.

## Reproducir desde los ZIP

Extraer el ZIP de fuentes en un directorio nuevo y el de diagnósticos en otro.
Las recetas están en `review/`; no requieren `.git`. El inventario usa revisión
null cuando faltan metadatos Git y no hereda el HEAD de un checkout contenedor.
Preparar localmente SDK 1.58a, el compilador SourcePawn y las dos imágenes fijadas;
no se incluyen SDK ni includes externos para compilar consumidores. Comprobar
sus versiones/hashes contra los manifiestos. Los contenedores siguientes no
tienen red y solo escriben en la extracción de fuentes.

Desde la raíz extraída, definir rutas absolutas existentes:

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

python3 scripts/stage-artifact.py . .build/p5-r18/reproducible-build/package .build/mi-candidato
EXTS_ARTIFACT_DIR="$PWD/.build/mi-candidato" bash scripts/ci-validate-artifact.sh
python3 scripts/prepare-review-artifacts.py --candidate .build/mi-candidato \
  --baseline /ruta/a/captura-baseline --output .build/mi-entrega --label mi-checkpoint
```

La captura baseline es una entrada privada previa, con raíz de SourceMod y
archivos propios de SteamWorks; no está dentro del ZIP de fuentes. Las salidas
build/integration deben ser nuevas. Estas recetas reproducen build, suites y
roundtrip de reversión local; los scripts de inventario externo, comparación con
DEV y recolección histórica describen la captura original y necesitan sus
repositorios/servidor, sin ejecutarse como parte de esta reproducción offline.
La revisión final prueba ambos comandos desde una extracción sin `.git`.
No se autoriza publicación ni operación del servidor.
