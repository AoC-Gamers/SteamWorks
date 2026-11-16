# P5-R20: integración de T26/T35 del candidato P5-R19

`develop`, base `adffed0cc90cad929183b7adc896a6f86ce281da`. Esta actualización queda
sin commit, push ni despliegue y **no opera servidores**. Incorpora la
[aceptación T26/T35](p5-r19-rollback-srcds-acceptance.md) del ZIP local R19 y repite
la validación integrada antes de generar cuatro paquetes nuevos.
**GC remoto positivo continúa pendiente; no se declara aceptación global nueva.**

## Aceptación incorporada y alcance

El seguimiento R19 instaló realmente el ZIP `b8a4a8cd…` dentro del guest detenido,
verificó todos sus miembros y ejecutó su payload en SRCDS x86 con LinuxGSM,
`competitive_dev`, puerto 27015 y árbol `addons/sourcemod_acceptance`. Luego aplicó
el ZIP rollback `55153d55…` en ese mismo árbol detenido y arrancó los bytes
originales, sin simular reversión mediante otro selector.

Ambos procesos conectaron a Steam y pasaron HTTP 200/cuerpo de 16 bytes/contexto/
callback/cierre. Se cargó un observador común y las extensiones habituales,
incluida SourceTVSupport: **no se acepta la pila completa de consumidores**.
Los dos quit fueron limpios en ese perfil mínimo. El SIGSEGV histórico del
baseline en R9 no se reprodujo y no se declara resuelto de forma general.
DEV quedó restaurado, conectado a Steam, con cuatro hashes y 1843 CFG intactos.

El ELF R20 reproduce exactamente el aceptado:
`72664c658f6d28e78a975ec98c96879dc641f96725206f514f159ea17125fcb3`.
El include mantiene
`969541624a2ba2f14aa07e040f96e6a50db40503baa21cba6206e0b1bd25aad8`.
La API pública sigue igual a P0 en firmas, tipos, registros y opcionalidad.
El ZIP candidato **R20 nuevo no se instala**: se conserva la aceptación del
payload idéntico y se verifican los nuevos paquetes localmente. El ZIP rollback
R20 es byte por byte el aplicado en R19, aunque su nombre de archivo cambie.

La [aceptación de buffers de revisión 12](review12-srcds-acceptance.md) conserva
372 checks, HTTP real y GC real vacío/proveedor local tipado, con esos mismos
ELF/include. Un proveedor local o una cola vacía no acreditan contenido recibido
del backend GC. Los demás ensayos runtime permanecen ligados a su propio hash;
no se transfieren automáticamente los resultados del ELF `839ff075…`.
La [matriz activa](p5/validation-results.json) distingue esas procedencias.

## Validación R20 repetida

- Quince suites x86 y comparación completa de API con P0 pasan. Incluyen registro
  tipado, binding 014 con SourceHook, init/teardown, streaming, macro Windows activa,
  aridad HTTP antigua, empaquetado y controles de rollback.
- ASan/LSan/UBSan nativos y VM offline, sin supresiones: 133 checks del hook GC,
  33 del lector HTTP estricto, 227 de forwards y 16 ciclos de propiedad de VM.
  La VM diagnóstica conserva su procedencia y no se instala en SRCDS.
- Build optimizado `-O3 -DNDEBUG -m32 -Werror`, con
  `SOURCE_DATE_EPOCH=1794700800`, reproduce los bytes aceptados. La fecha real de
  validación se registra separadamente de la fecha fijada del compilador.
- Tres consumidores, 19 harnesses, cuatro fixtures Connect, dos controles de
  include y los helpers ELF32 forward/capacidades/buffers compilan sin warnings.
  Los dos ejemplos HTTP actuales de la documentación también compilan.
- Las 66 pruebas Python y el build de Connect pasan; sus salidas permanecen en
  el scratch de SteamWorks. Sus fuentes/API no cambian ni se repite su runtime.
  Se comprueban 55 entradas SDK, 157 fuentes y el fixture de 733 claves stats.
- Se extraen los ZIP candidato/fuentes/diagnósticos y se verifican sus contenidos.
  Desde fuentes sin `.git` se repiten build y 15 suites; los diagnósticos extraídos
  pasan replay de 227 checks/16 ciclos. El parche completo de Connect reproduce
  todos los archivos de su commit destino en una extracción temporal.
- Captura baseline renovada por lectura y roundtrip de los ZIP reales R20 en
  filesystem desechable: hashes, ausencia de gamedata y CFG/plugin ajenos
  preservados. DEV mantiene el mismo PID, selector y cuatro hashes antes/después.

[Integración](p5/r20/integration.json),
[fuentes extraídas](p5/r20/source-extracted-integration.json),
[sanitizadores](p5/r20/asan-focused.log),
[SDK/harnesses](p5/r20/sdk-harness-checks.json),
[baseline por lectura](p5/r20/baseline-observation.json),
[rollback local](p5/r20/rollback-roundtrip.json).
El [CI existente](https://github.com/AoC-Gamers/SteamWorks/actions/runs/43889902957)
pasó siete jobs sobre `adffed0`; se conserva su captura, sin disparar CI para R20
ni convertir compilación Windows en aceptación runtime.

## Paquetes nuevos

Conjunto `.build/p5-r20/review/`, con manifiesto, verificación y `SHA256SUMS`:

| ZIP | Contenido |
|---|---|
| `steamworks-p5-r20-linux-x86-candidate.zip` | Solo ELF/include en addons; licencia, mapa y documentación conciliada |
| `steamworks-p5-r20-competitive-dev-rollback.zip` | Extensión/include habituales y contrato de ausencia de gamedata |
| `steamworks-p5-r20-source-review.zip` | Fuentes, tests, evidencia, atributos Git, recetas y parche Connect completo |
| `steamworks-p5-r20-vm-diagnostics.zip` | VM offline y receptores/helpers de laboratorio, fuera de addons |

Se preservan los ZIP anteriores y su evidencia. No se distribuyen SDK,
bibliotecas Steam/engine, CFG privadas, secretos o identidades. Candidato/fuentes
no incluyen SMX compilados de pruebas; estos se limitan a los diagnósticos.
Los binarios consumidores HTTP de tres argumentos siguen requiriendo recompilación.
El [rollback](p5-rollback.md) no sustituye una captura propia de Connect antes
de cualquier intervención futura sobre ese consumidor.

## Reproducción y límites

Extraer source-review y vm-diagnostics en directorios nuevos. Preparar SDK 1.58a,
compilador SourcePawn y las imágenes por digest indicadas, que no se distribuyen
en los ZIP. Desde la raíz extraída:

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

Estas dos recetas funcionan sin Git. El finalizador adjunto registra el
empaquetado del workspace original y requiere sus checkouts/capturas privadas;
no es un instalador portable ni una autorización para operar servidores.
[Checkpoint R19 anterior](p5/history/p5-r19-with-T26-T35-acceptance-validation-results.json).
No se infieren GC remoto positivo, Windows/x64 runtime, proveedores desconocidos,
todos los métodos SDK, ausencia global de fugas/OOM ni cierre del fallo histórico
del baseline. T26/T35 sigue limitado al perfil mínimo observado.

## Revisión final de R20

La [revisión final](p5/r20/final-review/results.json) corrige dos referencias de
verificación que todavía apuntaban a R19: el mapa de evidencia y el resultado
documentado del roundtrip. Ahora se contrastan los archivos de R20 y los 149
registros históricos que R20 incorpora, incluidos T26/T35. Dos controles en
copias desechables confirman el rechazo de evidencia o resultados alterados.

La matriz/manifiesto también separa `prior_in_place_evidence` (aplicación real
R19) de `current_in_place_evidence: null` y de la validación local R20. El ZIP
R20 no se presenta como instalado. Los cuatro paquetes y el rollback real se
verifican otra vez; las fuentes y recetas nativas mantienen sus hashes, por lo
que se conservan los resultados R20 ya ejecutados sin repetir pruebas de producto.
Los artefactos previos quedan en `.build/p5-r20/pre-final-review-request/`.

No cambia código nativo ni API. GC remoto positivo sigue pendiente; no se cierra
la matriz global ni el fallo histórico de salida del baseline. Sin commit, push,
operaciones de servidor o despliegue.
