# Changelog

Todos los cambios importantes de este proyecto se documentan en este archivo.

## [Unreleased]

### Análisis CXX estricto

- Argumento predeterminado de logging agrupado y atributos GNU de exportación
  con sintaxis C++11, conservando el límite de un MiB, el enlace C, la visibilidad
  de ambos factories y los exports Windows. Regresiones con el parser CXX real,
  controles negativos y ABI GCC x86.
- El análisis CXX rechaza errores de sintaxis sin recuperación parcial;
  no se ocultan atributos ni se excluyen fuentes para hacer pasar el scanner.

### Inicialización y propiedad del lector heredado

- Campos de DLC y secciones inicializados antes de leer; los propietarios de
  licencias, DLC y secciones rechazan copias superficiales que duplicaban la
  propiedad de memoria. Pruebas x86 con ASan/UBSan y controles negativos.
  El lector sigue fuera de la ruta activa de autenticación.
- Comprobaciones x86 de las macros reales de SourceHook sobre punteros a miembro
  de dos palabras y del snapshot de identidad en solicitudes de stats, sin
  alterar la API pública ni suprimir avisos del analizador.

### Análisis estático opcional

- Targets `make sonar` y `make sonar-cppcheck`, con ejecución mediante scripts,
  base de compilación x86 generada por AMBuild e integración con sonar-cxx.
  Configuración de ejemplo sin datos de infraestructura; token en `env.secret`
  ignorado por Git y pasado al scanner solo por entorno. Pruebas offline del
  tooling en Make e integración local, fuera del pipeline y de `make test-ci`,
  sin activar análisis remotos automáticos.
- El análisis obtiene identidad, macros e includes implícitos del compilador
  x86 con los flags y directorio reales y aporta un contexto común a los headers
  de CXX, sin crear objetos ni dependencias durante la consulta. Los errores de
  preprocesado/sintaxis de Cppcheck bloquean el envío de informes parciales;
  los límites del parser CXX se documentan.
- `make test-coverage` mide líneas reales de producción con GCC/gcov x86,
  conserva sanitizadores y controles negativos y excluye los hits de mutantes.
  Importación opcional en Sonar mediante un recibo ligado a las fuentes actuales,
  sin alterar umbrales ni presentar dobles locales como aceptación real.
- Cobertura Python real mediante `make test-python-coverage`, con coverage.py
  fijado en un venv de instalación explícita, subprocesos y recibo ligado a las
  fuentes. Importación conjunta con x86 en Sonar, sin remapear copias temporales
  ni modificar el quality gate.

### Protección de carga de la extensión

- Rechazo temprano de una segunda carga del mismo módulo por otro nombre, antes
  de reemplazar la identidad o los recursos del propietario activo. Evita el
  listener HTTP obsoleto de la ruta de doble inicialización; permite reintentar
  tras un fallo y cargar de nuevo después del desmontaje completo. Regresión
  x86 con ASan/UBSan y control negativo. Se realizaron pruebas locales SRCDS
  Linux x86 de ambos órdenes de carga, rechazo sin perder HTTP pendiente,
  recuperación del consumidor fallido, recarga y cierre limpio.

### Notificaciones asíncronas de stats

- Observador de descarga de caché independiente del ensayo activo: conserva el
  objetivo tras finalizar, desconectar o cambiar de mapa y permite borrarlo
  explícitamente, sin registrar identidades. Pruebas deterministas x86 separan
  cliente y GameServer para el ID compartido `1108`, con control negativo.
  Se realizaron pruebas locales SRCDS Linux x86: objetivo retenido después de
  finalizar y desconectar, sin descargas observadas durante cinco minutos.
  La recepción real de `OnUserStatsUnloaded` no está acreditada en L4D2 dedicado
  x86: Steam no ofrece un disparador documentado para forzarla. Se conserva la
  API por compatibilidad y la cobertura determinista; los consumidores no deben
  depender del evento para liberar sus recursos.
- Nuevos forwards `SteamWorks_OnUserStatsReceived(authid, result, appid)` y
  `SteamWorks_OnUserStatsUnloaded(authid)`, sin cambiar las firmas existentes.
  Resultados por solicitud con identidad/AppID capturados, agrupación de
  duplicados pendientes y cancelación al detener el contexto. Pruebas locales
  x86 con ASan/UBSan. Recepción de resultados, lecturas y recarga soportada
  comprobadas en una prueba local SRCDS Linux x86; descarga de caché aún sin
  recepción real observada.

### SDK 1.55

- SDK único con lock de headers y bibliotecas x86, guardas de selección en build,
  pruebas y CI, incluidas reconstrucciones incrementales. Registro de servidor
  adaptado a 014, con prefijo de autenticación independiente y controles
  negativos de versión/objeto equivocados.
- API SourcePawn conservada. Aceptación Linux SRCDS x86 del payload SDK 1.55:
  autenticación 014, identidad, stats enteros y float, HTTP/streaming,
  reinicialización en el mismo proceso y cierre limpio en perfil mínimo.
  Runtime Windows permanece pendiente.
- Reversión Linux x86 aceptada con baseline nuevo SDK 1.55 y finalizador de la sesión cliente Steam:
  instalación desde ZIP, Steam/HTTP y salida limpia del candidato y del baseline
  restaurado, incluido el cierre efectivo del contexto cliente activo.
  El fallo del baseline histórico conserva su propia atribución;
  no se transfiere aceptación de instalación a nuevos ZIP documentales.

### Documentación pública

- Las notas de Latest y Develop muestran fecha UTC de actualización, commit y
  enlace al intento de CI después de subir los paquetes. Se conservan tags y
  enlaces estables. CI recrea cada release como borrador y la publica tras cargar
  ambos ZIP, renovando su fecha de publicación; cambia los IDs internos y reinicia
  los contadores de los assets anteriores. Las notas se limitan al canal, fecha,
  commit y ejecución de CI.

- Entornos, rutas y paquetes de prueba descritos con nombres genéricos; metadatos
  de fixtures saneados con trazabilidad y sin ampliar aceptación runtime.
- Documentación por funcionalidad y fixtures protegidos; rutas y empaquetado
  actualizados sin cambios de API ni lógica nativa.
- Se retira el renderizador local y sus dependencias npm/Puppeteer. Los diagramas
  Mermaid permanecen en Markdown; la validación local de enlaces, referencias y
  límites de paquetes usa solo Python. Se eliminan las reglas ligadas a snapshots
  locales de migración y al número mínimo de diagramas. La validación documental
  no tiene un job de CI ni bloquea los builds Linux/Windows.

### Estructura de instalación

- JSON de herramientas junto a sus consumidores: `scripts/build/steamworks-sdk.lock.json`
  y `scripts/package/package-map.json`, sin cambiar su contenido. El mapa deja de
  incluirse en el ZIP instalable; ambos archivos permanecen en las fuentes.

- Binarios distribuidos con sufijo de motor: `steamworks.ext.2.l4d2.so` y
  `steamworks.ext.2.l4d2.dll`. La identidad lógica `steamworks.ext` y la API
  SourcePawn se conservan. La reversión captura también el nombre genérico
  anterior y registra las ausencias para restaurar exactamente el baseline.

- Include público ubicado en `addons/sourcemod/scripting/include/`; build,
  inventario y validación de paquetes actualizados. El diagnóstico general pasa
  a `tests/runtime/`, conservando los bytes y la API pública.

### Organización de pruebas

- Instrucciones agénticas consolidadas en un único `AGENTS.md` raíz, con mapa de
  rutas, niveles de evidencia y comandos Make obligatorios. Pruebas y validación
  de artefactos usan los mismos targets en local y CI; los scripts son backends.

- Fuentes, ejecutores y ayudantes reunidos en `tests/`; documentación y CI usan
  las nuevas rutas, conservando opciones, variables y controles existentes.

### Buffers HTTP y GC

- GC entrega al forward un snapshot de capacidad inicializada, copiando solo
  el mensaje válido; no lee salidas no garantizadas en errores. Mantiene
  capacidad, resultados históricos, supercesión y guarda de generación.
- El helper HTTP textual lee el cuerpo completo en memoria temporal antes de
  truncar; rechaza tamaños no representables y conserva salidas ante fallos.
- Regresiones previas reproducidas con proveedores controlados, matrices GC/HTTP,
  integración local x86 y sanitizadores sin supresiones. La API GC y sus pruebas
  locales se conservan por compatibilidad.

### Validación

- Pruebas x86 de fronteras, identidad, stats, HTTP, autenticación, lifecycle y
  SourceHook/VM; sanitizadores sin supresiones y controles negativos conservados.
- Instalación y reversión verificadas en un servidor aislado, con alcance ligado
  al hash del payload y al conjunto de consumidores ensayado.
- Build optimizado reproducible y paquetes/fuentes comprobados por separado.
  Compilación Windows no implica aceptación de runtime Windows.
- La API GC se conserva por compatibilidad; su funcionamiento depende del soporte
  del juego y del contexto Steam. Las pruebas locales cubren sus contratos de ABI,
  buffers y forwards. El SIGSEGV histórico del baseline no se cierra por una salida
  limpia de un perfil mínimo.

### Corregido

- Los natives de stats, identidad, licencias y grupos resuelven los índices de
  cliente directamente, con rango y conexión comprobados antes de operar. Se
  elimina la dependencia de una entidad del motor durante la conexión, conservando
  firmas y autorización. Pruebas locales x86 cubren slots sin entidad, límites,
  clientes ausentes/desconectados y rechazo de referencias de entidad.
- El snapshot de BeginAuthSession valida la reserva acolchada contra PTRDIFF_MAX
  antes de reservar/leer; CDetour valida la capacidad de su respaldo fijo antes
  de copiar. El build completo con --enable-optimize y -Werror pasa, con 88 checks
  de hooks y 318 de respaldo x86, sin suprimir comprobaciones. API pública intacta.
- El observador de inicialización intercepta el init desde el `CALL` acreditado del motor y conserva el detour
  compartido de SourceTVSupport. Preserva sus ajustes y el resultado original,
  con regresión ejecutable x86, selector estricto y sanitizadores sin supresiones.
  La aceptación posterior cubre el ELF optimizado `839ff075…` y conserva cada
  ensayo anterior atribuido a su propio binario.
- El finalizador cierra la sesión cliente Steam restante durante la descarga de la extensión,
  solo tras observar el shutdown original del gameserver y confirmar su pipe en
  cero. Evita el worker del SDK sobre memoria desmapeada al salir de SRCDS x86;
  tres ciclos aislados LinuxGSM y un cambio de mapa pasan. API pública conservada,
  pruebas deterministas y sanitizadores sin supresiones; baseline sin modificar.
- Los callbacks HTTP de streaming conservan ambas celdas del contexto aceptado
  por Steam. El proveedor x86 pierde los bits altos en headers/data; cada petición
  conserva ahora el valor completo sin modificarlo si el setter falla. Se añaden
  pruebas deterministas x86 y un control negativo; API pública sin cambios.
- La VM aislada de pruebas conserva sus motores con tipos concretos y completa
  el cierre del entorno al destruirlo. Se incorpora un parche SourcePawn fijado,
  sin alterar interfaces públicas ni reemplazar bibliotecas instaladas. ASan/LSan
  valida destrucción y marshalling en x86_64/x86, sin suprimir comprobaciones.
- El forward interno de autenticación conserva `Param_Array` y sus tres argumentos,
  y envía bytes compactos mediante `PushArray`. Se elimina el rechazo de tipo de
  `PushStringEx`, con copia alineada, padding cero, longitud en bytes, AccountID
  completo y sin copyback. La API pública no cambia. Pruebas con CForward real y
  VM SourcePawn aislada pasan en x86_64/x86; no se usan tickets reales ni SRCDS.
- El análisis binario identifica en una instancia aislada la entrada real `SteamInternal_GameServer_Init`
  y conserva su ABI legacy de seis argumentos. Un perfil Linux x86 comprueba
  build-id del engine/proveedor, exports/RVA, región ejecutable y prefijos antes
  de instalar el par de detours; no se aceptan firmas arbitrarias de gamedata.
- El módulo adquirido para detours se conserva hasta destruir los trampolines.
  El factory alternativo usa el export público declarado `SteamInternal_CreateInterface`.
- Se retiraron los helpers de memoria sin consumidores. El build completo GCC
  ya no aplica a C++ el flag C `-Wno-pointer-to-int-cast`.
- Se verificaron inicio, shutdown/reinicio en el mismo proceso y descarga/recarga
  en un SourceMod aislado, preservando la instalación original y sin cambiar SDK.
  La API pública no cambia; otras plataformas/binarios requieren su evidencia.
- Se concilió `GetClientSteamID` conservando para buffers positivos el conteo
  histórico que incluye el terminador y corrigiendo el comentario del include.
  Capacidad cero devuelve 0 sin escribir; tamaños negativos o direcciones de
  salida inválidas generan error antes de formatear. La firma pública no cambia.
- Se añadieron 19 pruebas deterministas de SteamID textual con el formateador
  real de AMTL, verificadas en x86_64/x86 con UBSan y añadidas a CI Linux.
- Los natives de IP pública rechazan direcciones no IPv4 o no establecidas antes
  de leer el miembro IPv4. El recorrido de octetos usa un índice entero; la
  variante de una celda conserva los 32 bits aunque su valor firmado sea negativo.
- Se validó la dirección base de salida IPv4 y se añadieron 30 casos deterministas
  ejecutados con char firmado/sin signo en x86_64 y x86 con UBSan, también en el
  flujo de CI Linux.
- Los detours de inicio/shutdown verifican sus precondiciones y ambas creaciones
  antes de habilitarse. Un fallo limpia los recursos preparados, deja el par
  desactivado y registra el motivo. El cierre es repetible y limpia trampolines.
- El callback legacy de inicio solo solicita hooks tras éxito del original y
  con interfaz válida; conserva sus seis argumentos y resultado. Se añadieron
  26 pruebas deterministas en x86_64/x86 con UBSan y un paso de CI Linux.
- `SteamWorks_RequestStats*` separa AppID de identidad: comprueba el AppID del
  contexto mediante Utils y no lo interpreta como universo. Mantiene las firmas
  públicas; AppID ajenos/desconocidos, identidad inválida y clientes bot/no
  autorizados devuelven false. Los usos legacy del argumento como universo
  requieren migración al AppID real.
- Se agregaron 34 pruebas deterministas de solicitudes con `CSteamID` real del
  SDK y servicios simulados; pasan junto a las 32 pruebas de lectores en x86_64
  y x86 con UBSan. No sustituyen la aceptación en servidor.
- Los cuatro lectores de stats conservan la salida SourcePawn cuando Steam
  devuelve fallo, aunque el proveedor escriba antes de fallar. Se inicializan
  temporales y se validan las direcciones de nombre/salida antes de consultar.
- Se agregaron 32 casos deterministas sobre los lectores reales con dobles de
  Steam/SourceMod, verificados en x86_64 y x86 con UBSan y añadidos a CI Linux;
  sin sustituir la aceptación de runtime descrita arriba.

### Agregado

- Se integraron las correcciones del forward de autenticación y de la VM
  SourcePawn en una validación histórica de diez suites por arquitectura. Se
  prepararon candidato, fuentes, diagnóstico aislado y reversión con manifiestos
  y checksums, conservando las entregas anteriores.
- La integración reúne las suites locales y el control automático del inventario público.
  Se preparan candidato Linux x86 y reversión con hashes, restauración de archivos
  comprobada y documentación por capacidad; no implica aceptación remota ni despliegue.
- El empaquetado incorpora `LICENSE` y su validación. El snapshot de revisión
  excluye SDK, módulos del juego y configuración privada.
- Se agregó el inventario inicial de API, consumidores, SDK y pruebas, junto con un
  extractor de contratos y referencias. Su baseline se conserva en
  `tests/fixtures/api/api-inventory-p0.json`; los límites actuales de runtime se
  describen en `docs/runtime/compatibility.md`.
- Se agregó el plugin local de validación agrupada, actualmente en
  `tests/runtime/steamwork_test.sp`.
- Se agregaron pruebas agrupadas en `steamwork_test.sp` para:
  - estado y conectividad
  - identidad
  - pertenencia a grupos
  - validación de licencias
  - metadata del servidor
  - requests HTTP
  - requests de stats y lectura de stats
- Se agregaron scripts auxiliares de compilación Linux y un `Makefile` en la raíz.
- Se agregó documentación técnica por funcionalidad bajo `docs/`, incluidos
  HTTP, stats y el schema de L4D2 en `docs/stats/schema.md`. El fixture del
  schema se conserva en `tests/fixtures/stats/GetSchemaForGame_550.json`.
- Se agregó `SteamWorks_GetHTTPResponseBodyString(...)` como helper HTTP orientado a texto, que:
  - agrega terminador nulo
  - reporta `written`
  - reporta `truncated`

### Cambiado

- Se actualizaron las referencias de documentación a la estructura actual:
  enlaces relativos, plugin de pruebas en `tests/runtime/`, rutas por plataforma,
  flujo de CI y gamedata opcional no incluido en el repositorio ni los paquetes.
- Se rehízo el workflow de GitHub Actions para compilar desde `AoC-Gamers/Steamworks-SDK`.
- Se reorganizó CI en jobs separados para:
  - `deps-exts-linux` y `deps-exts-windows`: preparación de dependencias
  - `build-exts-linux` y `build-exts-windows`: compilación
  - `release-linux` y `release-windows`: empaquetado y validación de artefactos
  - `publish`: publicación de releases con ambos ZIP
- Se actualizaron las versiones de GitHub Actions usadas por CI.
- Se limitaron los archivos de extensión e include del release Linux a:
  - `addons/sourcemod/extensions/steamworks.ext.2.l4d2.so`
  - `addons/sourcemod/scripting/include/steamworks.inc`
- Se agregó un artefacto separado para Windows con:
  - `addons/sourcemod/extensions/steamworks.ext.2.l4d2.dll`
  - `addons/sourcemod/scripting/include/steamworks.inc`
- Se actualizó la integración base de bootstrap/build de la extensión para compatibilidad con SourceMod 1.12 y Metamod:Source 1.12.
- Se actualizó el repositorio para enfocarlo en Linux 32-bit para L4D2 como plataforma principal.
- El flujo del proyecto usa exclusivamente Steamworks SDK 1.55 (`sdk_155`).
- Se normalizaron a minúsculas los nombres visibles de release:
  - `steamworks.ext.2.l4d2.so`
  - `steamworks.ext.2.l4d2.dll`
  - `steamworks.inc`
- Se actualizó la documentación de HTTP y sus ejemplos para distinguir entre:
  - lectura cruda del body
  - lectura textual con terminador nulo
- Se actualizaron los ejemplos de stats para usar una key verificada real de L4D2:
  - `Stat.GamesPlayed.Total`
- Se corrigió el flujo de envío HTTP para registrar callbacks solo cuando el request fue enviado correctamente.
- Se corrigió la resolución de interfaces de matchmaking y Game Coordinator para que los overrides de gamedata se apliquen realmente.
- Se actualizó la documentación del repositorio para describir:
  - el flujo de compilación Linux para L4D2
  - las bibliotecas requeridas en runtime
  - el contenido de los artefactos de CI
  - la cobertura actual de pruebas
- Se estandarizó internamente `extension/` con una pasada conservadora sobre:
  - lifecycle de la extensión
  - forwards
  - game server
  - requests HTTP
  - game data
  - hooks y detours
  - natives
  - utilidades de memoria
  - parsing de tickets Steam
- Se reorganizó físicamente `extension/` en subdirectorios semánticos:
  - `core/`
  - `natives/`
  - `hooks/`
  - `http/`
- Se homogeneizó el código interno para usar:
  - `nullptr`
  - inicialización de miembros en headers
  - helpers pequeños de cleanup/release
  - menos duplicación en callbacks y lifecycle
- Se corrigió el rename real de `Extension/` a `extension/` para evitar fallos de CI en Linux por diferencias de mayúsculas/minúsculas.
- Se tradujeron y normalizaron los comentarios y nombres de parámetros en `steamworks.inc`.

### Eliminado

- Se eliminó el workflow antiguo de Travis CI.
- Se eliminaron plugins de ejemplo heredados que ya no forman parte del paquete final:
  - `scripts/swag.sp`
  - `scripts/UpdateCheck.sp`
- Se eliminaron los natives, forwards e integración runtime no soportados de `ISteamFriends`, tras validar que la interfaz no es utilizable en el contexto del servidor dedicado de L4D2.
- Se eliminó la lógica temporal de prueba de schema/Web API del plugin local de test, después de mover ese uso a documentación estática.

## [1.2.4]

### Notas
- Baseline anterior del fork AoC antes del trabajo actual de modernización local.
