# P5-R14: integración de T23 aceptado en P5-R13

Rama `develop`, base `46e3ee05fc29ad090a1d6f7b1cb5e3dca9855350`.
**T23 pasa en su alcance U/I; P5 sigue sin aceptación global.** R14 repite build,
pruebas y empaquetado locales e incorpora la [aceptación T23 de R13](forward-consumers-srcds-acceptance.md).
No opera SRCDS, publica ni despliega. Los paquetes anteriores permanecen intactos.

El build optimizado x86 (`--enable-optimize -O3 -DNDEBUG -m32 -Werror`) reproduce
`839ff0751b04141031bdc6dffc48662d0804f52227f2c91015bd87c6b84cf192`, el ELF
[aceptado directamente en SRCDS](optimized-srcds-acceptance.md). La API pública
coincide con P0: firmas, registros, valores, typesets y opcionalidad.

El include de R14 incorpora el comentario corregido de `RestartRequested`:
`a3e5a7a220dd4d7b46dab3226b41c68f2eb24c4ddf4943f0b943f87566b4f2d5`.
Respecto al include del ZIP R13 cambia **solo ese comentario**; no se invierte el
comportamiento histórico ni se cambia una firma. El ELF y el ZIP rollback son
idénticos a los ejercitados; el payload completo difiere por el texto del include.
El refresco integrado verificó R14 localmente; el seguimiento T11 posterior
instala el payload del ZIP verificado en la copia aislada.

## Evidencia conservada por hash y alcance

| Evidencia del ELF `839ff075…` | Resultado |
|---|---|
| [Lifecycle/HTTP optimizados](optimized-srcds-acceptance.md) | Dos reinicializaciones en el mismo PID con SourceTVSupport; 25 completions, 18 bytes streaming/contexto, respuesta retenida a través de shutdown sin callback obsoleto en la ventana medida; quit limpio |
| Autenticación/identidad/stats | BeginAuthSession 014 y ambos forwards públicos, identidad y licencia de AppID 550; entero coincidente, 12 floats finitas en dos recorridos de 733 claves y 16 lecturas de claves inexistentes preservando salidas |
| [T15/T29 de R12](identity-negatives-srcds-acceptance.md) | T15 aceptado: capacidades 1/0/negativas, truncamiento, dirección VM inválida, conteo/NUL/canarios. T29 parcial: bot/desconectado y rechazo temprano por referencia inválida; faltan rutas de identidad validada en host real |
| [T16 de R11](t16-extension-srcds-acceptance.md) | Carga tardía y dos unload/recargas por consola sin consumidores/peticiones; desmapeo, restauración de hooks, SourceTVSupport y pipes preservados; HTTP/nuevo juego/quit correctos |
| [T20 de R10](t20-owner-srcds-acceptance.md) | Dos descargas de propietario con HTTP pendiente, handles propios 1→0, recarga antes de liberar y siete segundos sin callback antiguo por ciclo; probes nuevos y quit correctos |
| [T23 de R13](forward-consumers-srcds-acceptance.md) | Doce forwards gestionados reales con datos sintéticos; siete rondas, 672 despachos/4803 checks sin fallos; orden falso detectado. Mapeo de reinicio contrastado con 109 checks deterministas y desensamblado baseline/candidato |
| [T26/T35 de R9](p5-r9-rollback-srcds-acceptance.md) | ZIP reales instalados/restaurados en el mismo árbol detenido; baseline con archivos/configs/Steam/HTTP y 135 plugins correctos; candidato sale limpio, baseline conserva SIGSEGV final |

Los ensayos runtime usaron `competitive_dev`, LinuxGSM, copia aislada en 27015 y
flags/CFG habituales, sin contraseña añadida. Las pruebas humanas no escribieron
stats ni exportaron tickets, IDs o valores personales. T23 usó exclusivamente
argumentos sintéticos y retiró plugins ajenos solo de la copia de laboratorio.

T23 acredita el despacho de SourceMod/VM sobre los forwards registrados, incluidos
ET_Ignore/ET_Hook/ET_Event, orden y Stop, parámetros binarios, ticket sin copyback,
token con copyback secuencial y código de despacho separado del resultado agregado.
**No genera callbacks del SDK, no acredita GC disponible ni prueba un reinicio
real solicitado por Steam.** Esos límites no se convierten en aceptación de T11/T32.
El mapeo del hook conserva la convención del baseline: si el SDK solicita reinicio
y hay receptores, Handled/Stop mantienen true y Continue/Changed lo suprimen.

## Validación integrada repetida

- **15 suites x86 pasan.** `test-restart-forward-semantics` incorpora los 109 checks
  como regresión permanente del runner y añade un paso x86/UBSan al workflow.
  Actionlint pasa; CI remota no se ejecuta.
- 88 checks de hooks/lifecycle, 318 de respaldo CDetour y 27 escenarios owner/wrapper;
  142 de convivencia ejecutable, 21 de admisión y 18 de binding 014 con SourceHook.
- 227 checks CForward/VM, 16 ciclos de propiedad y 92 de contexto streaming, con sus
  controles negativos. ASan/LSan/UBSan activos sin supresiones, incluido el nuevo
  mapeo de reinicio; límites relevantes comprobados con `-O3 -DNDEBUG`.
- Build optimizado completo con warnings como errores y ELF idéntico al mapeado.
  Los 55 headers/dependencias del SDK coinciden con P0.
- Tres consumidores recompilados sin warnings, cuatro tablas de imports SMX y
  diez harnesses compilados sin warnings. Fixture de 733 claves idéntica a la
  aceptación optimizada. El helper ELF32 de T23 recompilado coincide con el que
  se cargó en el laboratorio; se conserva como fuente de prueba, fuera del runtime.
- Inventario renovado: 2696 archivos, 39 con referencias. Persisten dos llamadas
  HTTP externas de Connect con tres argumentos donde se requieren cinco; no se
  modifica ese repositorio ni se acepta su comportamiento completo.
- Doce pruebas del preparador, validación del árbol y ZIP extraído, integridad y
  roundtrip candidato→rollback en filesystem temporal: hashes/ausencia de gamedata
  restaurados y configuración ajena preservada.
- Replay de los diagnósticos x86 extraídos, sintaxis shell, enlaces y
  `git diff --check`. Captura de reversión y observación DEV solo por lectura:
  PID y cuatro hashes protegidos conservados durante este refresco.

[Logs R14](p5/r14/integration.json), [sanitizadores nativos](p5/r14/asan-native.log),
[reinicio instrumentado](p5/r14/asan-restart.log), [inventario](p5/api-inventory.json),
[consumidores](p5/consumers.json) y [matriz T01–T36](p5/validation-results.json).
La VM instrumentada fijada se reutiliza como diagnóstico offline; no se instala.

## Artefactos y reversión

Conjunto nuevo `.build/p5-r14/review/`, con manifiesto, verificación y `SHA256SUMS`:

| ZIP | Contenido |
|---|---|
| `steamworks-p5-r14-linux-x86-candidate.zip` | ELF32 optimizado, include con comentario corregido, licencia y documentación conciliada |
| `steamworks-p5-r14-competitive-dev-rollback.zip` | Extensión/include originales capturados por lectura y ausencia de gamedata registrada |
| `steamworks-p5-r14-source-review.zip` | Fuentes, workflow, scripts, tests de T23, parches, recetas y diff contra HEAD; sin SDK/deps privados |
| `steamworks-p5-r14-vm-diagnostics.zip` | VM y receptores sintéticos x86 con licencia/procedencia, fuera de addons |

El runtime contiene únicamente extensión/include. No incluye la extensión de
fixture, SMX de prueba, VM, configuraciones privadas, datos ni bibliotecas del juego.
La [reversión R14](p5-rollback.md) conserva SHA
`55153d557f3af7f71f1016c8c394dc74fe29e0cdc9511044c21de29e8bc6dffc`, idéntico al ZIP
R9 aplicado en sitio. T26/T35 mantienen ese alcance; la instalación del seguimiento
T11 no repite el protocolo completo T26/T35 ni aplica un ZIP rollback en sitio. **El baseline conserva su fallo de salida conocido.**

La [matriz anterior con T23](p5/history/p5-r13-with-T23-followup-validation-results.json),
los informes de aceptación y todos los paquetes anteriores quedan intactos.
R14 incorpora las fuentes de T23 y el comentario corregido que el ZIP R13 todavía
no contenía; esta integración no amplía la evidencia runtime ya observada.

El [seguimiento T11 de R14](interface-capabilities-srcds-acceptance.md) acepta la
matriz mínima del perfil con el payload real del ZIP R14: ocho interfaces
disponibles y Matchmaking no proporcionada. GC local, rechazo GC999 registrado
una vez y restauración tras arranque frío pasan; otras interfaces/HTTP conservados.
144 checks aceptados sin fallos, intentos inválidos del harness preservados aparte.
No se acredita backend GC remoto ni semántica completa de cada método.
No se regeneran los ZIP R14 en ese seguimiento.

## Pendientes vigentes

Cuatro casos siguen abiertos: **T25, T29, T32 y T33**.

- T25: comportamiento de consumidores precompilados/recompilados, incluidos
  H08/H09; dos llamadas HTTP externas de Connect pendientes de corrección.
- T29: cliente conectado válido sin autorización, fallback de AccountID validado
  sin SteamID64 y contradicción de ambas identidades validadas. El rechazo temprano
  por referencia inválida no acredita esas ramas; sus dobles deterministas pasan.
- T32: pérdida/recuperación de Steam sin destruir el runtime.
- T33: operación sostenida con recursos y rendimiento medidos.

Windows, x86_64 y otras parejas engine/SDK no heredan esta aceptación Linux x86.
La descarga forzada o desde callbacks activos no forma parte de T16 aceptado.

## Reproducir localmente

Usar dependencias e imágenes fijadas y salidas nuevas; las recetas completas se
incluyen en `review/` del ZIP de fuentes. El build usa `--enable-optimize -m32` y
las pruebas de VM Clang 14/ASan compartido, sin supresiones.

```sh
python3 scripts/test-integration.py --with-auth-binding --with-auth-forward \
  --with-sourcepawn-environment --output .build/mi-integracion-x86
python3 scripts/stage-artifact.py . .build/p5-r14/build/package .build/mi-candidato
EXTS_ARTIFACT_DIR="$PWD/.build/mi-candidato" bash scripts/ci-validate-artifact.sh
python3 scripts/prepare-review-artifacts.py --candidate .build/mi-candidato \
  --baseline .build/p5-r14/baseline --output .build/mi-entrega --label mi-checkpoint
```

Estas recetas no arrancan un servidor ni autorizan publicación o despliegue.
Todo permanece en develop, sin commit.
