# P5-R6: integración con T08 y SourceTVSupport

Rama `develop`, base `46e3ee05fc29ad090a1d6f7b1cb5e3dca9855350`.
La reconstrucción Linux i386 produce exactamente el ELF aceptado en T08:
`82347b99d5130bd411cdc0b35d215dc87fc82d394a2252ccd83d5d4940b6ca5a`.
API pública conservada. Este refresco ejecuta builds y pruebas locales sin red,
renueva la reversión por lectura y prepara paquetes de revisión. No arranca/detiene
el gameserver, instala candidatos, publica ni despliega. **P5 no tiene aceptación global.**

## Aceptaciones y atribución por binario

| Binario | Evidencia real conservada | Alcance actual |
|---|---|---|
| `82347b99…` (P5-R6) | [T08 con SourceTVSupport](t08-coexist-srcds-acceptance.md) | Dos reinicializaciones completas en PID 58894, originales de init/shutdown observados, un forward de conexión por ciclo, 25 casos HTTP correctos, 18 bytes streaming con contexto completo, petición retenida sin callback antiguo y quit sin SIGSEGV/nuevos cores |
| `82347b99…` (seguimiento humano R6) | [Auth/identidad/stats](p5-r6-auth-srcds-acceptance.md) | Payload del ZIP instalado, un humano, auth 014 y ambos forwards, identidad/licencia, enteros y dos scans float/missing-key correctos |
| `bc76b00d…` (P5-R3/R4/R5) | [Autenticación/identidad/enteros](p5-r3-auth-srcds-acceptance.md), [floats y claves inexistentes](p5-r4-stats-srcds-acceptance.md), [HTTP previo](http-stream-srcds-acceptance.md) | Histórica: humano autorizado y validación, buffers/licencia, stats por cliente/AccountID, 12 floats coincidentes y claves ausentes con salidas preservadas. No aprueba esas capacidades en el ELF nuevo |
| `ddfdf9bf…` | [Adaptador 014 anterior](auth014-srcds-acceptance.md) | Historia de una versión anterior, sin herencia entre hashes |
| Baseline `9ca37aad…` | [Reversión en sitio P5-R5](p5-r5-rollback-srcds-acceptance.md) | Restauración y arranque/Steam/HTTP aceptados en aquel ensayo; conserva su fallo de salida |

T08 queda aceptado en el nuevo ELF. T12 y HTTP T17/T18 conservan el mismo alcance
observado en esa aceptación; T21 incorpora la respuesta antigua retenida y su
ventana medida, además de las pruebas deterministas. El cambio de init conserva
la entrada del SDK para SourceTVSupport y observa el `CALL` acreditado del motor;
la [corrección y su frontera](t08-init-coexistence.md) mantienen la validación de
pareja binaria, contexto, destino relocado y rangos ejecutables.

La [aceptación humana posterior](p5-r6-auth-srcds-acceptance.md) cierra T01–T03
y los casos observados de identidad sobre el nuevo binario. Las aceptaciones
anteriores no se borran: la [matriz anterior con seguimientos](p5/history/p5-r5-with-t08-followups-validation-results.json)
y los paquetes previos permanecen intactos. El antiguo fallo de reinicialización
con SourceTVSupport sigue atribuido a `cd42f40c…`, no a este ELF.

## Validación integrada repetida

- Build completo x86 con warnings como errores; hash idéntico al módulo realmente
  mapeado en T08. SDK 1.58a intacto: 55 headers/dependencias contrastados con P0.
- **14 suites x86 pasan**, incluido inventario público: 47 natives, 11 forwards
  declarados, enums, typesets, registros y opcionalidad sin cambios.
- Convivencia: 142 comprobaciones ejecutables x86, 21 del selector y control
  negativo; wrappers/rollback: 27 escenarios. Desconocidos, punto alterado y
  errores de preparación/escritura siguen rechazándose.
- SourceHook 014: 18 comprobaciones y control negativo 015. CForward/VM: 227
  comprobaciones y 16 ciclos de entorno. Streaming: 92 comprobaciones y control
  negativo. ASan/LSan/UBSan permanecen activos sin supresiones, también en lifecycle,
  streaming y convivencia nativos.
- Tres consumidores recompilados sin warnings y cuatro tablas de imports SMX
  contrastadas con los natives registrados. Los seis harnesses runtime también
  compilan; el fixture de 733 claves se regenera idéntico. Compilar/importar no
  acredita comportamiento de todos los consumidores en el servidor.
- Inventario renovado: 2683 fuentes examinadas y 30 con referencias. Las dos
  llamadas HTTP externas de Connect con tres argumentos en lugar de cinco siguen
  pendientes; no se modifica ese repositorio.
- Doce pruebas del preparador; validación de árbol y ZIP extraído, integridad de
  archivos, roundtrip de ZIP candidato a ZIP de reversión en filesystem temporal
  y replay de los diagnósticos x86 extraídos. Se preserva una config ajena y se
  retira el gamedata inicialmente ausente.
- Workflow ampliado con la regresión de convivencia; actionlint, sintaxis shell,
  diferencias Git y checksums documentales comprobados. CI remota no ejecutada.

[Logs R6](p5/r6/integration.json), [sanitizadores nativos](p5/r6/asan-native.log),
[inventario](p5/api-inventory.json), [consumidores](p5/consumers.json) y
[matriz T01–T36](p5/validation-results.json). La VM instrumentada se reutiliza del
build x86 fijado; se prueba como diagnóstico aislado y no reemplaza la instalada.

## Artefactos y reversión

Conjunto nuevo `.build/p5-r6/review/`, con manifiesto, verificación y `SHA256SUMS`:

| ZIP | Contenido |
|---|---|
| `steamworks-p5-r6-linux-x86-candidate.zip` | ELF32 aceptado en T08, include, licencia y documentación actualizada |
| `steamworks-p5-r6-competitive-dev-rollback.zip` | Extensión/include originales capturados por lectura; ausencia de gamedata documentada |
| `steamworks-p5-r6-source-review.zip` | Fuentes propias, scripts, workflow, pruebas, parches y diff contra HEAD; sin SDK/deps privados |
| `steamworks-p5-r6-vm-diagnostics.zip` | VM y receptores sintéticos x86 con licencia/procedencia; fuera de `addons` |

El payload instalable contiene solo extensión e include; no se agregan CFG
privados, bases de datos, tickets, módulos Steam/engine ni VM. Los cuatro paquetes
son de revisión local, no una autorización de publicación o despliegue.
La [reversión R6](p5-rollback.md) conserva los hashes originales. Su roundtrip local
se prueba con el nuevo candidato; la aplicación en sitio del nuevo ZIP y su
arranque posterior no se repiten en esta tarea.

## Pendientes vigentes

- Ampliar los casos humanos/negativos y la matriz semántica: la
  [aceptación posterior P5-R6](p5-r6-auth-srcds-acceptance.md) ya valida un humano,
  ambos forwards, identidad/licencia y stats enteros/float/claves ausentes.
- Aceptar la reversión R6 en sitio: su payload de candidato del ZIP ya se instaló
  en el seguimiento humano, pero restaurar el selector original no aplica el ZIP
  de reversión. El ensayo R5 no sustituye la comprobación R6.
- Completar interfaces/GC y la matriz semántica de consumidores/forwards, así como
  los negativos de identidad todavía simulados.
- Carga tardía, unload de extensión en caliente y owner unload con petición
  pendiente; desconexión de Steam sin destruir el runtime, otras carreras de
  generación y operación sostenida/performance.
- Corregir externamente las dos llamadas HTTP de Connect. Windows y otras parejas
  de módulos están fuera de esta entrega exclusivamente Linux x86.

## Reproducir sin desplegar

Usar las dependencias fijadas, `-m32`, `SOURCEMOD_DIR`, `MMSOURCE_DIR`, `AMTL_DIR`,
`SPCOMP` y VM x86 instrumentada. Las suites VM usan Clang 14, ASan compartido y
`-fsized-deallocation -fno-pie -no-pie`; no se utilizan supresiones.

```sh
python3 scripts/test-integration.py --with-auth-binding --with-auth-forward \
  --with-sourcepawn-environment --output .build/mi-integracion-x86
python3 scripts/stage-artifact.py . .build/p5-r6/build/package .build/mi-candidato
EXTS_ARTIFACT_DIR="$PWD/.build/mi-candidato" bash scripts/ci-validate-artifact.sh
python3 scripts/prepare-review-artifacts.py --candidate .build/mi-candidato \
  --baseline .build/p5-r6/baseline --output .build/mi-entrega --label mi-checkpoint
```

Cada salida debe ser nueva. Las recetas de esta revisión acompañan al ZIP de
fuentes; no arrancan el servidor ni instalan la VM diagnóstica.

## Seguimiento posterior del paquete R6

La [aceptación humana P5-R6](p5-r6-auth-srcds-acceptance.md) cierra la repetición
de auth/identidad/stats en `82347b99…` y acredita su payload instalado desde el
ZIP. Incluye dos rondas de 733 claves, las mismas 12 floats coincidentes y las
cuatro salidas preservadas ante clave inexistente. Los paquetes anteriores y su
verificación de preparación no se regeneran; este resultado es un seguimiento
posterior, reflejado en la matriz actual. Reversión en sitio y aceptación global
continúan pendientes.
