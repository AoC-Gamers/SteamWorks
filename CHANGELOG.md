# Changelog

Todos los cambios importantes de este proyecto se documentan en este archivo.

## [Unreleased]

### Correcciones posteriores a P5-R18
- GC entrega al forward un snapshot de capacidad inicializada, copiando solo
  el mensaje válido; no lee salidas no garantizadas en errores. Mantiene
  capacidad, resultados históricos, supercesión y guarda de generación.
- El helper HTTP textual lee el cuerpo completo en memoria temporal antes de
  truncar; rechaza tamaños no representables y conserva salidas ante fallos.
- Regresiones previas reproducidas, proveedor HTTP estricto, matrices GC/HTTP,
  integración x86 y sanitizadores sin supresiones. Aceptación posterior SRCDS x86:
  372 checks sin fallos, HTTP real, GC real vacío y proveedor GC local tipado;
  mensaje GC remoto positivo pendiente. Quit limpio y DEV restaurado. P5-R18
  y OneDrive conservados como evidencia histórica.

### Validación

- Revisión final R20: guardas de evidencia/roundtrip ligadas al checkpoint R20,
  con controles negativos en copias temporales; referencias de reversión actual
  e histórica separadas. Artefactos previos preservados, payload/API intactos
  y GC remoto pendiente; sin operaciones de servidor ni publicación.

- P5-R20 integra T26/T35 del ZIP local R19, repite 15 suites x86 y sanitizadores
  sin supresiones, reproduce el payload aceptado y renueva cuatro ZIP/rollback.
  Conserva el alcance mínimo y el fallo de salida histórico del baseline sin
  cierre general. GC remoto pendiente; sin operaciones de servidor, commit,
  publicación ni despliegue en esta regeneración.

- Seguimiento T26/T35 R19: ZIP local instalado y revertido en el mismo SRCDS
  aislado con LinuxGSM. Candidato/baseline pasan Steam/HTTP y quit en perfil
  mínimo; el SIGSEGV histórico del baseline no se reproduce ni se cierra globalmente.
  DEV restaurado y 1843 CFG intactos; GC remoto pendiente y ZIP anteriores intactos.

- Revisión final R19: ejemplos HTTP coherentes con lectura raw completa y
  conteos en bytes; recetas portables separadas del empaquetado local. Artefactos
  revisados y anteriores preservados, sin cambios nativos/API ni cierre GC remoto.

- P5-R19 incorpora la aceptación de revisión 12, repite 15 suites x86 y
  sanitizadores sin supresiones, reproduce el ELF aceptado y renueva cuatro ZIP
  con reversión por lectura/roundtrip local. GC remoto positivo queda pendiente;
  evidencia histórica separada del nuevo ELF. Sin operaciones de servidor,
  commit, publicación ni despliegue.
- Revisión final R18: concilia tres campos auxiliares de CI con pasos exitosos
  de cc52305 y aclara que la comparación con fecha libre es histórica de R17.
  Evidencia original y primeros ZIP R18 preservados; paquetes y reversión
  comprobados de nuevo. Sin cambios nativos/de API, commit, push ni despliegue.
- P5-R18 incorpora las correcciones Windows de macro max y enlace tier0.lib,
  con CI de producto cc52305 verde para Linux y Windows. Repite 15 suites x86,
  sanitizadores sin supresiones, build optimizado idéntico, 18 harnesses y
  consumidores/Connect; conserva 33 casos aceptados y tres no aplicables al
  perfil Linux x86. Regenera los cuatro paquetes y la captura de reversión,
  incluye atributos de bytes auditados y parche completo de Connect contra su
  base original. ZIP anteriores intactos; actualización sin commit/push ni
  operación de servidores. CI Windows no implica aceptación runtime Windows.
- CI Windows R17: tras confirmar la compilación con la macro `max`, declara
  tier0.lib para los ocho imports de diagnóstico/allocator usados por tier1
  ConVar. Imports contrastados con la librería SDK; build Linux repetido idéntico.
  Sin cambios de API, runtime Linux o perfil de aceptación; sin despliegue.
- Seguimiento CI R17: corrige colisión con la macro Windows `max` en cuatro
  límites numéricos sin desactivar macros ni comprobaciones. Control negativo
  reproducido y regresión x86 con la macro activa, 15 suites y build optimizado
  idéntico pasan. Se preservan los ZIP offline anteriores al ajuste y se repite
  CI remoto; sin operación del servidor ni ampliación de aceptación a Windows.
- Revisión final R17 corrige reproducción del ZIP de fuentes desde extracción
  limpia, entrada explícita de VM diagnóstica e inventario sin `.git`/HEAD ajeno;
  concilia referencias antiguas de GC/forwards. Build idéntico y 15 suites desde
  el ZIP, reversión real archivada en filesystem temporal y privacidad verificadas.
  Se preservan los ZIP R17 previos y se regeneran los corregidos, sin cambios
  nativos/de API, commit ni operación de servidor.
- P5-R17 integra el cierre T25/T29 y conserva 33 casos aceptados/tres no aplicables
  al perfil Linux x86. Repite 15 suites y sanitizadores sin supresiones, build
  optimizado idéntico con fecha fijada (primer build solo fecha/build-id distinto),
  18 harnesses, tres consumidores, cuatro fixtures Connect y 66 pruebas Python.
  Regenera candidato/fuentes/diagnósticos/reversión, verifica ZIP extraído y
  roundtrip local, preservando archivos anteriores. API intacta, DEV contrastado
  por lectura; sin operación SRCDS, commit, publicación ni despliegue.
- Cierre T25/T29 de R16: préstamo, perfil Web API y grupo reales; rechazo
  preautenticado y control autorizado del mismo cliente. Connect externo corrige
  la lectura HTTP de cinco argumentos, contexto SDK al arranque y propiedad del
  reintento. 18 checks humanos/sociales, 104 checks Connect x86, 66 pruebas Python
  y 15 suites x86 pasan; frío/HTTP/ACK/mapa aceptados. Matriz del perfil Linux x86
  completa en sus límites, ZIP R16 intactos, DEV restaurado, sin commit ni producción.
- T33 de R16 acepta veinte minutos de carga HTTP mixta local y ventanas de
  reposo/recuperación: 1980 solicitudes completas/cerradas sin fallos,
  handles cero tras cada drenaje y recursos/latencia dentro de límites registrados
  antes del ensayo. CPU/frame medidos, quit limpio y DEV restaurado. Quince suites
  x86 repetidas pasan y observer sin warnings. ZIP R16 intactos, evidencia/fuentes
  separadas; T25/T29 siguen parciales, sin aceptación global ni producción/commit.
- T32 de R16 acepta un ciclo real de desconexión y recuperación Steam sin
  destruir runtime: mismo PID/módulos, nueve getters estables, respuesta HTTP
  retenida y peticiones nuevas durante/después del corte. 81 checks sin fallos,
  15 suites x86 repetidas y observer sin warnings. Filtros limitados al cgroup/UID
  del laboratorio retirados, DEV restaurado. Tres casos abiertos: T25/T29/T33.
  API/runtime y ZIP R16 intactos; evidencia/fuentes separadas, sin commit ni producción.
- P5-R16 integra T25 parcial de R15: seis variantes reales/37 checks y dos
  controles nuevos con includes antiguo/actual/22 checks humanos H08/H09.
  Conserva pendientes externos Connect y flujos completos, T29 parcial y T32/T33.
  Repite 15 suites x86, sanitizadores sin supresiones, build optimizado y catorce
  harnesses. Regenera cuatro paquetes/captura de reversión solo localmente;
  API/runtime y rollback aceptados idénticos, archivos anteriores intactos,
  sin commit, operación del servidor ni despliegue.
- T25 de R15 queda parcial tras probar seis variantes reales de consumidores
  (tres SMX existentes y tres recompilados): 37 checks, más 22 checks humanos
  H08/H09 en dos controles nuevos con includes antiguo/actual. Se repiten 15 suites
  x86. Persisten dos llamadas incompatibles en Connect y flujos completos sin
  cubrir; API/runtime intactos, DEV restaurado, cuatro casos abiertos y evidencia
  separada. Sin regenerar ZIP R15, commit ni producción.
- P5-R15 incorpora T11 aceptado para el perfil de R14: ocho interfaces disponibles,
  Matchmaking ausente y GC local/rechazo/restauración probados; sin inferir GC
  remoto ni todos los métodos. Repite 15 suites x86, tres consumidores, once
  harnesses, ambos helpers y sanitizadores sin supresiones. Regenera cuatro ZIP
  y reversión por lectura, conservando API, payload extensión/include y rollback
  ejercitados. Cuatro casos abiertos; paquetes anteriores intactos, sin commit ni
  despliegue.
- T11 de R14 acepta la matriz mínima de nueve familias en SRCDS x86: ocho
  disponibles y Matchmaking ausente para gameserver. GC local, rechazo de GC999
  una vez y restauración pasan; 144 checks aceptados sin fallos. Se conservan los
  intentos inválidos del harness y se repiten 15 suites x86. No se infiere GC
  remoto; API/runtime intactos, DEV restaurado, cuatro casos abiertos. ZIP R14
  intactos y seguimiento separado, sin commit ni producción.
- P5-R14 integra T23 aceptado de R13 e incorpora sus fixtures y el comentario
  corregido de RestartRequested, sin cambiar firmas ni comportamiento. Repite
  15 suites x86, tres consumidores, diez harnesses, helper ELF32 y sanitizadores
  sin supresiones. Regenera cuatro ZIP y reversión por lectura; ELF/rollback
  aceptados idénticos, include con cambio documental. Cinco casos abiertos;
  paquetes previos intactos, sin commit ni despliegue.
- T23 de R13 acepta despacho sintético sobre los doce forwards reales en SRCDS
  x86: 672 despachos y 4803 checks sin fallos; orden incorrecto detectado. Repite
  14 suites x86 y añade 109 checks del mapeo del hook de reinicio. Se corrige su
  comentario preservando firmas y convención histórica. DEV restaurado, cinco
  casos abiertos, ZIP R13 intactos; evidencia/fuentes de seguimiento separadas.
- P5-R13 incorpora T15 aceptado y T29 parcial de R12; seis casos globales abiertos.
  Repite 14 suites x86, build optimizado, tres consumidores, ocho harnesses y
  ASan/LSan/UBSan sin supresiones. Renueva cuatro ZIP y reversión por lectura,
  conservando API, ELF y rollback aceptados. El rechazo temprano por referencia
  inválida no acredita la rama conectada sin autorización; tampoco se acepta
  inconsistencia/fallback validado en host real. Sin commit ni despliegue.
- Seguimiento R12 en SRCDS x86: T15 acepta límites de buffers; T29 amplía
  negativos y sigue parcial. Harness sin IDs en logs, 40 comprobaciones finales
  sin fallos; quit y DEV restaurados. ZIP R12 intactos, seis casos abiertos.
- P5-R12 incorpora la aceptación T16 y el observador independiente. Repite 14
  suites x86, build optimizado, consumidores, siete harnesses y sanitizadores sin
  supresiones; renueva cuatro ZIP, documentación y captura de reversión. Conserva
  T16/T20/T26/T35 por hash y alcance, siete casos abiertos y el fallo de salida del
  baseline. Paquetes previos intactos, sin commit ni desplegar.
- T16 acepta carga tardía sobre gameserver conectado y dos ciclos de descarga/
  recarga por consola sin consumidores ni peticiones: desmapeo, hooks restaurados,
  pipes Steam y SourceTVSupport conservados, HTTP/nuevo juego/quit correctos.
  Solo se añade un harness; DEV restaurado, API/ELF/ZIP R11 intactos, sin commit
  ni producción. No certifica fuerza o unload desde callbacks activos.
- P5-R11 incorpora la aceptación T20 y su harness actualizado. Repite 14 suites
  x86, build optimizado, consumidores, seis harnesses y sanitizadores sin
  supresiones; renueva cuatro ZIP, documentación y captura de reversión. Mantiene
  T20/T26/T35 por hash, ocho casos globales abiertos y el fallo del baseline al
  salir. Paquetes previos intactos, sin commit ni desplegar.
- T20 acepta dos ciclos de descarga del plugin propietario con HTTP retenido y
  recarga antes de liberar la respuesta antigua: handles propios reclamados,
  probes nuevos correctos, sin callback obsoleto en siete segundos por ciclo.
  Quit limpio y DEV restaurado; solo cambia el harness, API/extensión intactas.
- P5-R10 concilia T26/T35 aceptados con los ZIP reales R9. Repite 14 suites x86,
  build optimizado y sanitizadores sin supresiones; regenera documentación, cuatro
  ZIP y captura de reversión. Runtime y ZIP rollback conservan sus hashes probados;
  nuevo ZIP candidato solo verificado localmente. Baseline con fallo de salida y
  pendientes globales conservados, sin commit ni desplegar.
- P5-R9 reproduce el ELF optimizado `839ff075…` aceptado directamente en SRCDS:
  dos reinicializaciones con SourceTVSupport, HTTP/streaming, auth/identidad/stats
  humanos y quit limpio. Repite 14 suites x86 y sanitizadores, renueva cuatro ZIP,
  documentación y reversión local. El seguimiento posterior acepta T26/T35 con
  ZIP reales, restauración/arranque/Steam/HTTP; el baseline conserva su SIGSEGV al
  salir. Aceptación global pendiente; paquetes anteriores intactos, sin commit ni
  despliegue en producción.
- P5-R7 concilia la aceptación humana de P5-R6 con T08 para el mismo ELF:
  repite 14 suites x86 y sanitizadores, regenera documentación, cuatro paquetes
  y reversión locales. Conserva paquetes previos y pendientes de aceptación global;
  sin commit ni despliegue.
- P5-R6 incorpora el ELF aceptado en T08 con SourceTVSupport: repite 14 suites
  x86 y sanitizadores sin supresiones, renueva paquetes/reversión locales y
  conserva atribución de autenticación/identidad/stats al binario anterior.
  Sin commit ni despliegue; aceptación global todavía pendiente.
- P5-R5 incorpora la aceptación directa de 12 claves float y claves inexistentes
  del payload P5-R4 en SRCDS x86. T03 pasa en el alcance observado; se renuevan
  validación local, documentación, artefactos y reversión sin despliegue ni commit.
- P5-R3 concilia la autenticación real del adaptador 014 en su binario R3 y la
  aceptación HTTP streaming del candidato final. Repite 13 suites x86 y regenera
  artefactos/reversión locales; conserva pendientes por candidato y no aprueba P5
  globalmente. Sin commit, publicación ni despliegue en esta actualización.

### Corregido
- El snapshot de BeginAuthSession valida la reserva acolchada contra PTRDIFF_MAX
  antes de reservar/leer; CDetour valida la capacidad de su respaldo fijo antes
  de copiar. El build completo con --enable-optimize y -Werror pasa, con 88 checks
  de hooks y 318 de respaldo x86, sin suprimir comprobaciones. API pública intacta.
- T08 observa el init desde el `CALL` acreditado del motor y conserva el detour
  compartido de SourceTVSupport. Preserva sus ajustes y el resultado original,
  con regresión ejecutable x86, selector estricto y sanitizadores sin supresiones.
  La aceptación posterior cubre el ELF optimizado `839ff075…` y conserva cada
  ensayo anterior atribuido a su propio binario.
- T08 cierra la sesión cliente Steam restante durante la descarga de la extensión,
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
- P4 identifica en `competitive_dev` la entrada real `SteamInternal_GameServer_Init`
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
  flujo de CI Linux (ejecución remota pendiente).
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
  ejecución remota de CI y runtime pendientes.

### Agregado
- P5-R2 reúne T36 y el parche de la VM en una nueva validación de diez suites
  por arquitectura. Regenera candidato, fuentes, diagnóstico aislado y reversión
  con manifiestos y checksums, conservando las entregas anteriores sin desplegar.
- P5 integra las suites locales y el control automático del inventario público.
  Se preparan candidato Linux x86 y reversión con hashes, restauración de archivos
  comprobada y documentación por capacidad; no implica aceptación remota ni despliegue.
- El empaquetado incorpora `LICENSE` y su validación. El snapshot de revisión
  excluye SDK, módulos del juego y configuración privada.
- Se agregó el inventario P0 de API, consumidores, SDK y pruebas bajo
  `docs/runtime-compatibility/`, junto con un extractor de contratos y referencias.
  Registra un harness compilado, un baseline nativo fallido y validación runtime
  pendiente; no implica cambios de API ni aceptación operativa.
- Se agregó `scripting/steamwork_test.sp` como plugin local de pruebas para validación agrupada de SteamWorks.
- Se agregaron pruebas agrupadas en `steamwork_test.sp` para:
  - estado y conectividad
  - identidad
  - pertenencia a grupos
  - validación de licencias
  - metadata del servidor
  - requests HTTP
  - requests de stats y lectura de stats
- Se agregaron scripts auxiliares de compilación Linux y un `Makefile` en la raíz.
- Se agregó soporte para seleccionar directorios de Steamworks SDK como `sdk_158a`.
- Se agregó documentación técnica bajo `docs/`, incluyendo:
  - `docs/README.md`
  - `docs/ISteamFriends.md`
  - `docs/ISteamHTTP.md`
  - `docs/ISteamUserStats.md`
  - `docs/GetSchemaForGame_550.md`
  - `docs/TestCoverage.md`
- Se agregó el archivo local de referencia del schema:
  - `docs/GetSchemaForGame_550.json`
- Se agregó `SteamWorks_GetHTTPResponseBodyString(...)` como helper HTTP orientado a texto, que:
  - agrega terminador nulo
  - reporta `written`
  - reporta `truncated`

### Cambiado
- Se actualizaron las referencias de documentación a la estructura actual:
  enlaces relativos, plugin de pruebas en `scripting/`, rutas por plataforma,
  flujo de CI y gamedata opcional no incluido en el repositorio ni los paquetes.
- Se rehízo el workflow de GitHub Actions para compilar desde `AoC-Gamers/Steamworks-SDK`.
- Se reorganizó CI en jobs separados para:
  - `deps-exts-linux` y `deps-exts-windows`: preparación de dependencias
  - `build-exts-linux` y `build-exts-windows`: compilación
  - `release-linux` y `release-windows`: empaquetado y validación de artefactos
  - `publish`: publicación de releases con ambos ZIP
- Se actualizaron las versiones de GitHub Actions usadas por CI.
- Se limitaron los archivos de extensión e include del release Linux a:
  - `addons/sourcemod/extensions/steamworks.ext.so`
  - `addons/sourcemod/scripting/include/steamworks.inc`
- Se agregó un artefacto separado para Windows con:
  - `addons/sourcemod/extensions/steamworks.ext.dll`
  - `addons/sourcemod/scripting/include/steamworks.inc`
- Se actualizó la integración base de bootstrap/build de la extensión para compatibilidad con SourceMod 1.12 y Metamod:Source 1.12.
- Se actualizó el repositorio para enfocarlo en Linux 32-bit para L4D2 como plataforma principal.
- Se cambió el flujo del proyecto para usar Steamworks SDK `1.58a` mediante `sdk_158a`.
- Se normalizaron a minúsculas los nombres visibles de release:
  - `steamworks.ext.so`
  - `steamworks.ext.dll`
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
