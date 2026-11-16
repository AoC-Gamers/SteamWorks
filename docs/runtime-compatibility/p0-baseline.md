# P0: inventario y baseline de SteamWorks

Fecha: 2026-11-14. Rama: `develop`. Commit:
`46e3ee05fc29ad090a1d6f7b1cb5e3dca9855350`, idéntico a la base del pedido
documental v1.0. Responsable de esta ejecución: Codex.

El inventario está realizado. El harness compila; el intento de build nativo
del baseline falla con GCC 12 por una opción de compilación. No se validó una
instalación L4D2 ni se aprobó un perfil ABI. Los hallazgos de código de este
informe son evidencia estática E1; no se presentan como fallos reproducidos en
producción.

## Alcance y archivos de evidencia

Se conservaron las correcciones documentales locales anteriores. En P0 no se
cambiaron `extension/`, `scripting/`, el SDK, las firmas públicas ni la
configuración de build. No se hicieron commits, releases ni despliegues.
No hay archivos `AGENTS.md` aplicables encontrados en el proyecto ni en sus
directorios ancestros examinados.

- [API completa](p0/api-inventory.json): firmas, argumentos, retornos, valores de
  enums, callbacks, registro C++, natives opcionales y hashes de fuentes.
- [Consumidores](p0/consumers-inventory.json): archivos, líneas, símbolos,
  cantidad de argumentos, hashes, revisión/estado de cada checkout y diferencias
  entre includes.
- [SDK y toolchain](p0/sdk-toolchain.json): revisión y huellas del SDK, selectores
  de interfaces y entorno del intento de compilación.
- [Resultados](p0/validation-results.json): comprobaciones realizadas y estados
  de las pruebas pendientes del plan T01–T36.
- [Extractor](../../scripts/inventory-steamworks.py): permite repetir el
  inventario sin ejecutar Steam ni cargar binarios del juego.

Los hashes identifican los archivos leídos, incluidos posibles cambios locales
en consumidores. El commit del checkout por sí solo no describe un árbol sucio.
El paquete del pedido permanece como planificación externa; no se instalaron
sus plantillas JSON como gamedata.

## API pública y registro C++

Fuentes: [include](../../scripting/include/steamworks.inc),
[GameServer natives](../../extension/natives/gsnatives.cpp),
[stats](../../extension/natives/ssnatives.cpp),
[HTTP](../../extension/http/swhttprequest.cpp),
[GC](../../extension/natives/gcnatives.cpp).

| Superficie | Resultado del inventario |
| --- | --- |
| Biblioteca / archivo de extensión | `SteamWorks` / `steamworks.ext` |
| Natives públicos | 47: GameServer/identidad 15, stats 6, HTTP 25, GC 1 |
| Natives registrados | 47; ninguna declaración sin registro ni registro sin declaración |
| Forwards declarados | 11; todos registrados |
| Forwards registrados | 12; uno adicional sin declaración pública |
| Enums | 5, con 163 miembros y valores explícitos/resueltos en el JSON |
| Tipos de callback | 4 typesets HTTP, cada uno con 3 variantes |
| Natives marcados opcionales | 45 de los 47 declarados |

Enums: `EUserHasLicenseForAppResult` (3), `EResult` (103), `EHTTPMethod` (8),
`EHTTPStatusCode` (44), `EGCResults` (5). No se renumeró ningún valor.
Los typesets son `SteamWorksHTTPRequestCompleted`,
`SteamWorksHTTPHeadersReceived`, `SteamWorksHTTPDataReceived` y
`SteamWorksHTTPBodyCallback`.

Observaciones adicionales a H01–H10:

- **Forward no declarado:** `SteamWorks_BeginAuthSession` se registra en
  [swgshooks.cpp](../../extension/hooks/swgshooks.cpp). Usa `Param_Array`, mientras
  su envío utiliza `PushStringEx` binario. T36 debe conciliar el marshalling;
  no se capturaron tickets para este inventario.
- **Opcionalidad incompleta:** `SteamWorks_SetMapName` y
  `SteamWorks_GetHTTPRequestWasTimedOut` faltan en
  `__ext_SteamWorks_SetNTVOptional`. El efecto sobre carga sin extensión requiere
  prueba SourceMod; el listado es una discrepancia estática confirmada.
- **Grupo asíncrono:** el comentario de `GetUserGroupStatus*` describe pertenencia,
  pero el native devuelve el resultado de `RequestUserGroupStatus`; el resultado
  de pertenencia llega mediante `SteamWorks_OnClientGroupStatus`.
- **Restart:** el include describe `Plugin_Handled` como bloqueo del restart;
  el hook asigna `Result >= Pl_Handled` al booleano retornado por
  `WasRestartRequested`. Requiere conciliar contrato y prueba T23; no se cambió.

## Consumidores y compatibilidad

Se recorrieron 2.670 archivos `.sp`/`.inc` visibles para `rg --files --hidden`
bajo el directorio local `GitHub`, excluyendo dependencias, SDK, build, temporales,
vendors y archivos ignorados. Hubo 19 archivos con referencias: 13 con llamadas
o handlers y 6 con declaraciones, incluido el include canónico.

La búsqueda es léxica: no prueba que una rama sea alcanzable, que todos los
entrypoints se compilen, ni que esos plugins estén instalados. No incluye plugins
solo precompilados, servidores remotos ni fuentes fuera del directorio recorrido.

| Checkout / entrypoint | Uso encontrado |
| --- | --- |
| `AoC-L4D2-Competitive`, `AoC.sp` | IP pública, grupos, validación de cliente y conexión Steam |
| `AoC-L4D2-Competitive`, `l4d2_restart_sv.sp` | Forward de restart |
| `BanSystem/BanSystem-Sourcepawn`, `bansystem_core.sp` y módulos | HTTP, headers, timeout, cuerpo textual y transporte privado |
| `Connect/Connect-Sourcepawn`, `connect.sp` y módulos | HTTP, subida desde archivo, TLS, User-Agent y disponibilidad de la extensión |
| `L4D2-Family-Share`, `l4d2_familyshare_core.sp` y `profiles.sp` | Validación de dueño/jugador y HTTP para perfiles |
| `SteamWorks`, `steamwork_test.sp` | Estado, metadata, identidad, grupos, licencias, stats, HTTP y 6 handlers globales |
| `L4D1_2-Plugins` | Copia antigua del include; sin llamadas/handlers encontrados en este barrido |

BanSystem tenía cambios locales al inventariarlo; se registró el hash de cada
archivo observado. No se modificaron esos repositorios.

### Incompatibilidad adicional: HTTP textual de 3 y 5 argumentos

Las copias de `steamworks.inc` de Connect y AoC declaran:

```sourcepawn
native bool SteamWorks_GetHTTPResponseBodyString(Handle request, char[] body, int length);
```

El include de este proyecto requiere además `int &written, bool &truncated`.
Connect hace llamadas de tres argumentos en `connect/http.inc:224` y
`connect/ingest.inc:569`. La implementación actual lee `params[4]` y `params[5]`
sin comprobar `params[0]`. Esta combinación debe tratarse como incompatible
hasta definir y probar una política; no se infiere un resultado runtime concreto.
Una prueba mínima de recompilación con tres argumentos y el include canónico
falló con `error 034` por ausencia del argumento 4; ese fallo esperado se
registró como comprobación negativa de compilación, no como ejecución SourceMod.

BanSystem y Family Share usan la firma de cinco argumentos. El include de
Family Share es idéntico byte a byte al canónico; BanSystem conserva sus mismas
firmas. La copia antigua de `L4D1_2-Plugins` también declara
`SteamWorks_ForceHeartbeat` y `SteamWorks_SetAdvertiseServerActive`, ausentes en
esta extensión, y no declara el helper textual. No se promete compatibilidad con
esa superficie antigua completa.

### H08: AppID frente a identidad — confirmado en código

`RequestStatsAuthID` pasa `params` al constructor con posición de universo `2`;
por ello el segundo argumento público `appid` se interpreta como universo.
La ruta cliente normal obtiene el universo de la autenticación; su fallback
reutiliza el constructor numérico y comparte la discrepancia.

El único consumidor de `RequestStats*` encontrado fuera de declaraciones es
el harness: pasa `550` en ambas variantes. No se encontraron consumidores locales
que usen deliberadamente el argumento como universo. Esto no descarta plugins
legacy remotos o precompilados.

**Decisión pendiente para P1-B:** conservar dos argumentos, separar identidad y
AppID y acordar la validación frente al AppID del contexto. P0 no aprueba
rechazar AppID ajenos ni ignorarlos silenciosamente. T01/T25/T29/T30 siguen
pendientes de fixtures y/o servidor.

### H09: conteo textual — confirmado en código

`ISourceMod::Format` documenta bytes escritos sin terminador; el native suma uno.
Se contrastó con el header de SourceMod usado dentro del contenedor y con la
implementación del checkout local (`Format` → `FormatArgs` → `SafeVsprintf`).
El include de SteamWorks dice excluir el terminador.

Solo el harness llama `GetClientSteamID` en el barrido; registra el conteo y
comprueba `written > 0`, sin exigir una longitud exacta. Falta prueba de buffers
mínimos/pequeños/inválidos y compatibilidad precompilada. La política de conservar
el retorno histórico o corregirlo explícitamente permanece pendiente.

## SDK, dependencias y baseline de compilación

`sdk_158a/public/steam` contiene 53 archivos. Todos coinciden byte a byte con
`sdk_158a` del checkout limpio de `AoC-Gamers/Steamworks-SDK`, commit
`a7aaa2754d9cda244d0cd9d7523c1144a1c472d2`. La librería Linux32 de enlace y
`steam_api.lib` también coinciden; sus huellas están en el snapshot.
Los headers de la carpeta local predeterminada `sdk/` tienen el mismo manifiesto
que `sdk_158a/`. Se eligió explícitamente `sdk_158a` para el intento de build.
No se copiaron headers privados a los entregables.

| Interfaz compilada | Selector del SDK 1.58a |
| --- | --- |
| `ISteamClient` | `SteamClient020` |
| `ISteamGameServer` | `SteamGameServer015` |
| `ISteamUtils` | `SteamUtils010` |
| `ISteamNetworking` | `SteamNetworking006` |
| `ISteamGameServerStats` | `SteamGameServerStats001` |
| `ISteamHTTP` | `STEAMHTTP_INTERFACE_VERSION003` |
| `ISteamMatchmaking` | `SteamMatchMaking009` |
| `ISteamGameCoordinator` | `SteamGameCoordinator001` |

Son selectores de headers, no interfaces observadas en un servidor.

El host es elementary OS 8.1, Linux x86_64, GCC 13.3.0 y Python 3.12.3.
Una prueba mínima de enlace con `g++ -m32` falló por ausencia de multilib
(`Scrt1.o`, `crti.o`, `libstdc++`, `libm`, `libgcc`). No se instalaron paquetes
en el host ni se descargaron dependencias nuevas.

Se aprovechó el contenedor local `sourcetvmanager-builder:local`, identificado
por digest en el JSON, con red deshabilitada y fuentes montadas solo para lectura.
Tiene Debian 12, GCC 12.2.0, Python 3.11.2 y AMBuild API 2.2.4 (el metadata del
paquete Python dice `2.0`; no es la versión de API requerida por `configure.py`).

| Dependencia del contenedor | Revisión |
| --- | --- |
| HL2SDK L4D2 | `c2badaa4f7e763a92427ac11202c349385810cc2` |
| SourceMod 1.12 | `2e229b111534b1be007dc3bd9acfcf2fc472e893` |
| Metamod:Source 1.12 | `75dd7b26fe92e535a11f0318ead73c3e05130d0b` |
| AMBuild | `01212cb57c96561f664b6dfb2ee10e66de6f81e4` |

No se observaron modificaciones versionadas en esas dependencias; AMBuild tiene
`AMBuild.egg-info/` sin seguimiento. Las revisiones de submódulos se registraron
también en el snapshot. Los scripts habituales siguen ramas mutables; estas
revisiones describen esta ejecución, no un pin agregado al CI.

Intento, dentro del contenedor:

```bash
make build-exts-linux LINUX_DEPS_DIR=/opt/sm-deps VENV_DIR=/opt/sm-deps/.venv STEAMWORKS_SDK_NAME=sdk_158a
```

**Resultado: falló, exit 2 de make.** AMBuild configura, pero GCC rechaza
`-Wno-pointer-to-int-cast` al compilar C++ y `-Werror` lo eleva a error.
La opción proviene de `AMBuildScript`. No se alteraron flags para ocultar el
resultado ni se corrigió el build dentro de P0. No se produjo una extensión
final para empaquetar. La salida usa una carpeta denominada `linux-x86_64`
aunque los comandos añaden `-m32`; al no existir ELF final, ese nombre no
demuestra la arquitectura de un artefacto.

## Capacidades y criticidad por dependencia

Esta clasificación describe consumidores y riesgo para planificar; la extensión
actual no implementa todavía la máquina de estados propuesta.

| Capacidad / dependencia | Consumidor y comportamiento sin interfaz | Criticidad para la siguiente fase |
| --- | --- | --- |
| SteamClient | Resolver raíz; `IsLoaded` solo comprueba su puntero | Base de las adquisiciones Steam; `IsLoaded` no certifica todas las capacidades |
| GameServer | Estado, IP, metadata, grupos, licencias y hooks; normalmente false/0, licencias `NoAuth` | Relevante para AoC y autenticación; revisar política por consumidor |
| Identidad textual | SourceMod y construcción de `CSteamID` | Conciliar H09 sin cambiar silenciosamente el retorno |
| GameServerStats | Solicitudes/lecturas; false/0 sin interfaz | Solo harness localizado; H01/H08 prioritarios |
| HTTP | Handles y callbacks; creación devuelve handle inválido y operaciones false/0 sin interfaz | Uso directo por BanSystem, Connect y Family Share; no es prescindible para esos consumidores |
| Game Coordinator | Native devuelve `NotLoggedOn` sin interfaz; componente de hooks separado | Sin consumidor funcional localizado; soporte runtime pendiente |
| Utils, Networking, Matchmaking | Getters definidos, sin llamadas externas a esos getters dentro del árbol de extensión | Superficie interna sin uso localizado; no declarar soporte funcional por existir el getter |
| Detours InitSafe/Shutdown | Enganche de inicio/cierre y restauración de hooks GameServer | Revisar H02/H04/H10 antes de modificar ABI o cierre |
| Gamedata | Opcional; carga solo si existe en el servidor | No versionado ni empaquetado; no inventar firmas para completar el árbol |

## Resolución, callbacks y propietarios

El [resolver](../../extension/core/swgameserver.cpp) toma primero
`g_pSteamClientGameServer` si existe. Como alternativa consulta gamedata y luego
exports para `SteamGameServerInternal_CreateInterface` y
`SteamInternal_CreateInterface`. Usa `SteamGameServer_GetHSteamUser`/
`SteamGameServer_GetHSteamPipe` para las interfaces derivadas y cachea los punteros.
Las siete familias derivadas admiten overrides de versión sin adaptar el tipo
compilado: H03 permanece pendiente. `GetLibraryPath` conserva una ruta estática
con override `LibSteamAPI`.

| Recurso | Propietario / registro actual | Retirada e incertidumbre registrada |
| --- | --- | --- |
| Interfaces Steam | Punteros prestados en `SteamWorksGameServer` | `Reset` borra cachés; no destruye objetos del motor |
| Configuración | `SteamWorksGameData` / SourceMod GameConfig | `CloseGameConfigFile` en destructor |
| Bibliotecas abiertas | Resolver y constructor de detours | `CloseLibrary` tras resolver; no implica propiedad del runtime del motor |
| 6 callbacks globales Steam | Miembros `STEAM_GAMESERVER_CALLBACK` de `SteamWorksForwards` | Destructores del SDK desregistran callbacks; no hay coordinador explícito por shutdown del runtime |
| 6 forwards de core | `SteamWorksForwards` | Liberados en su destructor |
| 3 hooks GameServer y 3 forwards | `SteamWorksGSHooks` | Add/Remove y espera `OurGameFrameHook`; destructor consulta nuevamente el getter, sin guardar instancia enganchada |
| 3 hooks GC y 3 forwards | `SteamWorksGCHooks` | Add/Remove y espera `OurGCGameFrameHook`; cierre separado del GameServer |
| 2 detours | `SteamWorksGSDetours` | `Destroy` al descargar; `EnableDetour` devuelve void y la creación puede devolver null |
| Tipo de handle HTTP | `SteamWorksHTTP` | `RemoveType`; destrucción del objeto de petición diferida mediante `AddFrameAction` |
| Request y forwards HTTP | `SteamWorksHTTPRequest`, handle con identidad del plugin | Libera request y forwards al destruir; usa getter HTTP durante destrucción |
| 3 `CCallResult` por request | Miembros de `SteamWorksHTTPRequest`, asociados al enviar con éxito | El destructor del SDK cancela; cierre dentro de callback, frame diferido y shutdown requieren T20/T21 |

La descarga destruye componentes en orden explícito. El detour de shutdown
visible retira hooks GameServer, resetea cachés y llama al original; no coordina
de forma explícita hooks GC o requests pendientes. No se concluye use-after-free
sin ejecución de los casos correspondientes. Pérdida de conexión y cambio de
mapa no se clasifican automáticamente como destrucción del runtime.

`SteamWorksMemUtils`, `ResolveSymbolInt` y `GetOffsetFromVTable` solo aparecen
en sus propios `.h/.cpp`: no tienen consumidores localizados ni están en
`extension/AMBuilder`. Son candidatos a retirada en P4; no se eliminaron en P0.

## Pruebas disponibles y resultados reales

- Hay un harness manual: [steamwork_test.sp](../../scripting/steamwork_test.sp).
  Registra siete comandos administrativos: `sm_swtest_status`, `http`, `identity`,
  `group`, `license`, `servermeta` y `stats` (todos con prefijo `sm_swtest_`).
  Tiene logging para seis forwards; no cubre validación de cliente ni GC.
- Se compiló sin warnings con SourcePawn `1.12.0.7259`, usando las herramientas
  Linux ya presentes en Family Share y el include de este checkout. Salida:
  `.build/p0/steamwork_test.smx`. Esto valida compilación, no ejecución en Steam.
- El harness consulta stats inmediatamente y tras un timer de 2 segundos;
  no espera una notificación de disponibilidad. No tiene centinelas de fallo
  ni inyección determinista de errores de detours/interfaces.
- El ensayo HTTP imprime cuerpos en logs y permite URL arbitraria. Para pruebas
  futuras debe usarse un endpoint controlado sin datos privados; no se ejecutó
  ninguna petición HTTP del harness en P0.
- No hay suite unitaria de la extensión ni fixtures de ABI/errores en el árbol
  versionado. La CI existente construye y valida paquetes, sin ejecutar SRCDS.
- `ci-validate-artifact.sh` comprueba presencia de binarios y archivos declarados;
  no carga el ELF ni valida ABI. No se ejecutó sobre un paquete ficticio.
- No se observó un proceso local `srcds_linux` ni un contenedor de gameserver
  activo. Los módulos cargados, build del juego, versiones runtime de SM/MM,
  gamedata instalada y plugins precompilados quedan por identificar en un
  servidor de pruebas seleccionado. Windows también carece de entorno validado.

Los logs locales se conservaron en `.build/p0/`; sus huellas y los comandos
ejecutados figuran en el registro de resultados. La cobertura histórica de
`docs/TestCoverage.md` se mantiene como antecedente; no se convirtió en nuevos
resultados exitosos de T01–T36.

## Referencias externas y límites de ABI

Se consultó [Netprops](https://netprops.l4d2node.org/) el 2026-11-14: es un
catálogo de SendTables/propiedades de entidades. No aporta los prototipos ni
la compatibilidad binaria de las interfaces Steam usadas por esta extensión.

Se consultó [Binaries](https://binaries.l4d2node.org/) en la misma fecha: lista
decompilaciones Ghidra de `engine_srv.so`, `matchmaking_ds_srv.so` y
`server_srv.so`. El [índice de engine](https://binaries.l4d2node.org/b/engine_srv.so/)
ofrece referencias a clases y callbacks Steam del motor, útiles para orientar P4.
No se vinculó ese catálogo con hashes de un módulo cargado de AoC, ni se
aprobaron firmas, offsets o el prototipo legacy de `InitSafe` a partir de él.

## Salida de P0 y condiciones para continuar

H08/H09 quedaron confirmados estáticamente y con consumidores locales
inventariados. H07 tiene corregida su descripción documental; instalación y
paquete siguen pendientes de validación. H06 queda acotado a código sin uso
localizado. H01/H02/H03/H04/H05/H10 conservan sus pruebas pendientes.

Antes de migrar contratos deben resolverse la política AppID, el conteo SteamID
y la coexistencia HTTP de tres/cinco argumentos. Es posible avanzar por separado
con H01 y pruebas deterministas, conservando esos límites. Para obtener un
baseline nativo hace falta resolver la incompatibilidad del build con GCC o
ejecutarlo en un toolchain compatible identificado; la ejecución actual queda
fallida. La aceptación operativa exige un servidor L4D2 Linux x86 identificado,
plugins precompilados/recompilados, módulos reales y pruebas de cierre.
