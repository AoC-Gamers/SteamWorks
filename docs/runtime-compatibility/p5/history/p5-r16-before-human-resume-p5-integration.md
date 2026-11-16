# P5-R16: integración de T25 parcial de P5-R15

Rama `develop`, base `46e3ee05fc29ad090a1d6f7b1cb5e3dca9855350`.
**T25 permanece parcial; P5 sigue sin aceptación global.** R16 incorpora la
[aceptación acotada T25 de R15](consumer-compat-srcds-acceptance.md): seis variantes
reales (tres SMX existentes y tres recompilados) pasan 37 checks; dos fixtures
nuevos compilados con includes antiguo/actual pasan 22 checks humanos H08/H09.
Estos fixtures se distinguen de los consumidores históricos precompilados.

Persisten dos llamadas HTTP externas incompatibles en Connect y flujos completos
de negocio/dependencias sin cubrir. Connect completo no se cargó ni modificó.
Sus formas de llamada se reprodujeron mediante rechazo de compilación y dos
excepciones nativas esperadas, conservando las salidas y los logs normales.
La primera clasificación incorrecta de la ruta del fixture y su corrección
quedan preservadas. No se infiere cobertura completa de BanSystem/Connect,
licencias prestadas, perfil Web API ni grupos.

R16 repite 15 suites x86, sanitizadores nativos y VM offline sin supresiones,
build optimizado, tres consumidores y catorce entrypoints de harness sin warnings,
y ambos helpers. Incorpora las tres nuevas fuentes de fixtures de consumidores.
El ELF conserva `839ff0751b04141031bdc6dffc48662d0804f52227f2c91015bd87c6b84cf192`
y el include `a3e5a7a220dd4d7b46dab3226b41c68f2eb24c4ddf4943f0b943f87566b4f2d5`.
La API pública coincide con P0. Extensión/include son idénticos al payload R15
instalado en T25; **el nuevo ZIP R16 se verifica solo localmente**.
No se opera SRCDS, publica ni despliega en esta actualización. Se renuevan cuatro
paquetes y la captura de reversión por lectura, conservando los anteriores.

## Seguimiento T32 posterior al empaquetado

[T32 de R16](connection-srcds-acceptance.md) pasa un ciclo real de pérdida de red
Steam y recuperación automática, con el mismo PID y módulos: `IsLoaded` true,
nueve getters estables y respuesta HTTP retenida; HTTP nuevo durante/después del
corte. 81 checks sin fallos y 15 suites x86 repetidas. El ZIP R16 verificado se
instaló para este seguimiento; el empaquetado original fue local. Se retiraron
filtros exactos por cgroup/UID y laboratorio, y se restauró DEV. Los ZIP R16
permanecen intactos; fuentes/evidencia nuevas se entregan separadas.
No se acredita operación sostenida, stats humanos positivos ni HTTP/GC remoto.

## Seguimiento T33 posterior al empaquetado

[T33 de R16](soak-srcds-acceptance.md) pasa un ensayo medido de tres minutos de
reposo, veinte de carga HTTP local mixta y tres de recuperación, tras calentamiento.
Pasan los límites preregistrados de recursos, latencia, drenaje y logs; CPU/frame
medidos frente a reposo. Mismo proceso/módulos, quit limpio y DEV restaurado.
No se infieren varias horas, capacidad de producción, jugadores o backends remotos.
Los ZIP R16 permanecen intactos y el nuevo harness/evidencia se entrega separado.

## Evidencia conservada por hash y alcance

| Evidencia del ELF `839ff075…` | Resultado |
|---|---|
| [Lifecycle/HTTP optimizados](optimized-srcds-acceptance.md) | Dos reinicializaciones en el mismo PID con SourceTVSupport; 25 completions, 18 bytes streaming/contexto, respuesta retenida a través de shutdown sin callback obsoleto en la ventana medida; quit limpio |
| Autenticación/identidad/stats | BeginAuthSession 014 y ambos forwards públicos, identidad y licencia de AppID 550; entero coincidente, 12 floats finitas en dos recorridos de 733 claves y 16 lecturas de claves inexistentes preservando salidas |
| [T11 de R14](interface-capabilities-srcds-acceptance.md) | Nueve familias clasificadas mediante getters del candidato y consultas tipadas; ocho disponibles y Matchmaking null. GC local/GC999 rechazado una vez/defaults restaurados; otras interfaces/HTTP preservados. Cinco rondas, 144 checks aceptados sin fallos |
| [T25 de R15](consumer-compat-srcds-acceptance.md) | Parcial: seis variantes reales/37 checks; dos controles nuevos antiguo/actual/22 checks humanos H08/H09; faltan Connect y flujos completos. DEV restaurado; no rollback ZIP en sitio |
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
  catorce harnesses compilados sin warnings. Fixture de 733 claves idéntica a la
  aceptación optimizada. Los helpers ELF32 de T23 y T11 recompilados coinciden con los
  cargados en sus laboratorios; se conservan como fuentes de prueba, fuera del runtime.
- Inventario renovado: 2701 archivos, 44 con referencias. Persisten dos llamadas
  HTTP externas de Connect con tres argumentos donde se requieren cinco; no se
  modifica ese repositorio ni se acepta su comportamiento completo.
- Doce pruebas del preparador, validación del árbol y ZIP extraído, integridad y
  roundtrip candidato→rollback en filesystem temporal: hashes/ausencia de gamedata
  restaurados y configuración ajena preservada.
- Replay de los diagnósticos x86 extraídos, sintaxis shell, enlaces y
  `git diff --check`. Captura de reversión y observación DEV solo por lectura:
  PID y cuatro hashes protegidos conservados durante este refresco.

[Logs R16](p5/r16/integration.json), [sanitizadores nativos](p5/r16/asan-native.log),
[reinicio instrumentado](p5/r16/asan-restart.log), [inventario](p5/api-inventory.json),
[consumidores](p5/consumers.json) y [matriz T01–T36](p5/validation-results.json).
La VM instrumentada fijada se reutiliza como diagnóstico offline; no se instala.

## Artefactos y reversión

Conjunto nuevo `.build/p5-r16/review/`, con manifiesto, verificación y `SHA256SUMS`:

| ZIP | Contenido |
|---|---|
| `steamworks-p5-r16-linux-x86-candidate.zip` | ELF32 optimizado, include ya aceptado en R14, licencia y documentación conciliada |
| `steamworks-p5-r16-competitive-dev-rollback.zip` | Extensión/include originales capturados por lectura y ausencia de gamedata registrada |
| `steamworks-p5-r16-source-review.zip` | Fuentes, workflow, scripts, tests de T11/T23/T25, parches, recetas y diff contra HEAD; sin SDK/deps privados |
| `steamworks-p5-r16-vm-diagnostics.zip` | VM y receptores sintéticos x86 con licencia/procedencia, fuera de addons |

El runtime contiene únicamente extensión/include. No incluye la extensión de
fixture, SMX de prueba, VM, configuraciones privadas, datos ni bibliotecas del juego.
La [reversión R16](p5-rollback.md) conserva SHA
`55153d557f3af7f71f1016c8c394dc74fe29e0cdc9511044c21de29e8bc6dffc`, idéntico al ZIP
R9 aplicado en sitio. T26/T35 mantienen ese alcance; la instalación de T11 en R14
no repite el protocolo completo ni aplica un ZIP rollback en sitio. La integración
R16 repite el roundtrip en filesystem temporal; el seguimiento T25
instaló el candidato R15 y restauró DEV, sin renovar la aceptación de rollback. **El baseline conserva su fallo de salida conocido.**

La [matriz R15 con T25 parcial](p5/history/p5-r15-with-T25-partial-followup-validation-results.json),
los informes de aceptación y todos los paquetes anteriores quedan intactos.
R16 incorpora las fuentes de T25 y conserva las de T11, compiladas sin warnings; el helper
coincide con el ELF32 cargado en R14. La evidencia y los errores de fixture iniciales
se conservan completos, sin ampliar las capacidades observadas. El caso de GC
rechazado no acredita desconexión de Steam sin destruir el runtime de T32, y sus
consultas locales no equivalen a entrega/recepción de mensajes del backend GC.
La ausencia de Matchmaking no se extrapola a otros contextos o versiones.

## Pendientes vigentes

Dos casos siguen abiertos: **T25 y T29**.

- T25 parcial: seis variantes reales y H08/H09 con includes antiguo/actual pasan
  en sus rutas documentadas. Restan dos llamadas HTTP externas de Connect y los
  flujos completos de negocio/dependencias aún no ejercitados.
- T29: cliente conectado válido sin autorización, fallback de AccountID validado
  sin SteamID64 y contradicción de ambas identidades validadas. El rechazo temprano
  por referencia inválida no acredita esas ramas; sus dobles deterministas pasan.

Windows, x86_64 y otras parejas engine/SDK no heredan esta aceptación Linux x86.
La descarga forzada o desde callbacks activos no forma parte de T16 aceptado.

## Reproducir localmente

Usar dependencias e imágenes fijadas y salidas nuevas; las recetas completas se
incluyen en `review/` del ZIP de fuentes. El build usa `--enable-optimize -m32` y
las pruebas de VM Clang 14/ASan compartido, sin supresiones.

```sh
python3 scripts/test-integration.py --with-auth-binding --with-auth-forward \
  --with-sourcepawn-environment --output .build/mi-integracion-x86
python3 scripts/stage-artifact.py . .build/p5-r16/build/package .build/mi-candidato
EXTS_ARTIFACT_DIR="$PWD/.build/mi-candidato" bash scripts/ci-validate-artifact.sh
python3 scripts/prepare-review-artifacts.py --candidate .build/mi-candidato \
  --baseline .build/p5-r16/baseline --output .build/mi-entrega --label mi-checkpoint
```

Estas recetas no arrancan un servidor ni autorizan publicación o despliegue.
Todo permanece en develop, sin commit.
