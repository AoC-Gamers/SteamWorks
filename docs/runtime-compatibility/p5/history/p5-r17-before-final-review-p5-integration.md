# P5-R17: integración del cierre T25/T29 de R16

Rama `develop`, base `46e3ee05fc29ad090a1d6f7b1cb5e3dca9855350`.
La [matriz T01–T36](p5/validation-results.json) cierra sus 33 casos aplicables al
perfil Linux L4D2 x86; T13/T14/T34 no aplican. No quedan casos pendientes dentro
del alcance original. La aceptación técnica no autoriza publicación o despliegue.

## Cierre incorporado

[T25/T29 de R16](last-two-srcds-progress.md) aporta 18 checks reales: licencia
prestada, propietario distinto, perfil Web API con propietario desconectado,
grupo autorizado y cliente conectado/no autorizado que después acepta stats con
el mismo serial. La invariante del proveedor SourceMod realmente cargado y las
pruebas deterministas cubren las alternativas defensivas de identidad; no se
presentan como identidades inconsistentes inducidas en una cuenta real.

Connect corrige sus dos llamadas HTTP antiguas mediante un lector de cinco
argumentos y separa presencia de la extensión/contexto SDK. Un reintento de
asignación único evita cadenas duplicadas y se cancela al terminar mapa/plugin.
El SMX `0b6bfaa5…` pasó arranque frío con seis proveedores reales, HTTP200, ACK
competitivo sintético y cambio de mapa en el mismo proceso con HTTP200 posterior.
Los 104 checks SourcePawn de lector/contexto/timers pertenecen a ese ensayo R16;
R17 recompila sus fixtures y verifica los hashes de sus fuentes, sin repetirlo
sobre el servidor. Se conservan los fallos iniciales y su corrección.

[T32](connection-srcds-acceptance.md) conserva un ciclo real de pérdida y
recuperación Steam, 81 checks y runtime/interfaces/HTTP estables.
[T33](soak-srcds-acceptance.md) conserva tres minutos de reposo, veinte de carga
mixta local y tres de recuperación, 1980 requests, recursos/latencia medidos y
límites preregistrados. No se extrapolan horas ni capacidad de producción.

Las demás aceptaciones se conservan por hash y alcance: autenticación 014 y
stats optimizados, T11 capacidades/GC local, T15 buffers, T16 recarga quiescente,
T20 propietario HTTP, T23 forwards y T26/T35 instalación/reversión.
La [matriz anterior con T25/T29 cerrados](p5/history/p5-r16-with-T25-T29-closed-validation-results.json)
y los paquetes R16 permanecen intactos.

## Validación integrada nueva

- Quince suites x86 pasan, incluida comparación de API con P0; ASan/LSan/UBSan
  nativos y VM offline sin supresiones. Se repiten los checks de convivencia de
  init, streaming, mapeo de reinicio, forwards y propiedad de la VM.
- Build optimizado `-O3 -DNDEBUG -m32 -Werror`. Con `SOURCE_DATE_EPOCH=1794700800`
  reproduce exactamente el ELF aceptado `839ff0751b04141031bdc6dffc48662d0804f52227f2c91015bd87c6b84cf192`.
  El primer build con fecha del día solo difería en un byte de `__DATE__` y los
  veinte bytes del build-id; [comparación preservada](p5/r17/build-date-comparison.json).
  La fecha fijada es la del candidato, no la fecha de ejecución de este checkpoint.
- Tres consumidores, dieciocho harnesses runtime y cuatro fixtures Connect
  compilan sin warnings. Dos controles con includes antiguo/actual compilan;
  la llamada antigua de tres argumentos se rechaza con error 034 esperado.
- Las 66 pruebas Python de Connect y su build pasan. Sus fuentes coinciden con
  las del SMX aceptado; no se atribuye identidad binaria a una recompilación con
  timestamp distinto. El inventario renovado no tiene desajustes de aridad.
- Se verifican 55 entradas SDK, 152 fuentes del productor, imports SMX, fixture
  de 733 claves stats y ambos helpers ELF32 de T11/T23. Árbol candidato y ZIP
  extraído se validan; reversión archivada se ensaya en filesystem desechable.
  Los diagnósticos extraídos repiten 227 checks de forward y 16 ciclos de VM.
- Se renueva la captura baseline por lectura y se contrasta DEV antes/después:
  mismo PID, selector, módulo y cuatro hashes protegidos; sin laboratorio activo.
  No se inicia, reinicia o modifica ningún servidor en esta actualización.

Logs nuevos en [p5/r17](p5/r17/integration.json), [replay de diagnósticos](p5/r17/diagnostic-replay.log)
y comprobaciones DEV [antes](p5/r17/DEV-before.json)/[después](p5/r17/DEV-after.json). Los resultados runtime anteriores
no se reescriben. CI remoto no ejecutado; todo permanece sin commit.

## Artefactos nuevos y reversión

Conjunto `.build/p5-r17/review/`, con manifiesto, verificación y `SHA256SUMS`:

| Archivo | Contenido y alcance |
|---|---|
| `steamworks-p5-r17-linux-x86-candidate.zip` | Solo extensión/include dentro de addons, licencia, mapa y documentación conciliada |
| `steamworks-p5-r17-competitive-dev-rollback.zip` | Extensión/include originales capturados por lectura; ausencia de gamedata registrada |
| `steamworks-p5-r17-source-review.zip` | Fuentes, tests, evidencia, parches y recetas; corrección externa Connect como diff y nuevos helpers/tests, sin SDK/includes ajenos |
| `steamworks-p5-r17-vm-diagnostics.zip` | VM offline x86 y receptores sintéticos con licencia/procedencia; fuera de addons |

El ZIP candidato R17 nuevo se valida localmente; no se instala. ELF/include son
idénticos al payload R16 aceptado. El rollback conserva exactamente el SHA
`55153d55…` aplicado en sitio en R9; la [receta de reversión](p5-rollback.md) distingue
ese ensayo previo del roundtrip local nuevo. El baseline mantiene su SIGSEGV al
salir: no se certifica su cierre limpio. No se empaquetan CFG privados ni claves.

La corrección de Connect sigue sin commit en su repositorio develop y se incluye
para revisión, fuera del payload de SteamWorks. Cualquier consumidor HTTP de
tres argumentos requiere recompilación con el contrato actual. Una futura
intervención que cambie Connect necesita capturar y restaurar también su propio
SMX/configuración; el ZIP rollback de SteamWorks no sustituye esa captura.

## Límites vigentes

La matriz es del perfil engine/Steam/SourceMod Linux x86 observado. No certifica
Windows/x64, versiones desconocidas, otro proveedor de identidad, todos los
métodos SDK, GC remoto, Core remoto ni todo el negocio/partidas de los consumidores.
T16 es serial/quiescente y T20 usa ventanas acotadas; no hay descarga forzada con
callbacks activos ni prueba universal de ausencia de fugas SDK. T33 no es carga
de varias horas. Estos límites no se convierten en casos abiertos del alcance
original. Sin commit, publicación ni despliegue.

## Reproducir localmente

Los scripts y recipes fijados están en `review/` del ZIP de fuentes. Usar salidas
nuevas e imágenes por digest; no instalar la VM diagnóstica en SRCDS.

```sh
python3 scripts/test-integration.py --with-auth-binding --with-auth-forward \
  --with-sourcepawn-environment --output .build/mi-integracion-x86
SOURCE_DATE_EPOCH=1794700800 bash review/run-reproducible-build.sh
python3 scripts/stage-artifact.py . .build/p5-r17/reproducible-build/package .build/mi-candidato
EXTS_ARTIFACT_DIR="$PWD/.build/mi-candidato" bash scripts/ci-validate-artifact.sh
python3 scripts/prepare-review-artifacts.py --candidate .build/mi-candidato \
  --baseline .build/p5-r17/baseline --output .build/mi-entrega --label mi-checkpoint
```

Las recetas no autorizan publicación ni operación del servidor.
