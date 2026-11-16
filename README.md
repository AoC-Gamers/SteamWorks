# SteamWorks

Extensión de SourceMod para exponer funciones de **SteamWorks** a
**SourcePawn** en servidores dedicados de **Left 4 Dead 2**.

Este fork está orientado principalmente a servidores Linux de 32 bits y permite
usar desde plugins SourcePawn varias capacidades del runtime Steam disponible
en el gameserver, incluyendo:

- estado de conexión del servidor con Steam;
- VAC y public IP;
- metadata visible del servidor;
- reglas personalizadas;
- validación de clientes;
- consulta de licencias;
- consulta de grupos;
- stats de usuarios;
- requests HTTP mediante `ISteamHTTP`;
- interacción básica con Game Coordinator;
- forwards de eventos Steam relevantes para el servidor.

No es una librería genérica de Steam para cualquier contexto. Es una extensión
nativa pensada para el runtime real de servidores Source/SourceMod, con foco en
L4D2.

## Origen y créditos

Este repositorio deriva del proyecto **SourcePawn SteamWorks**, originalmente
desarrollado por Kyle Sanderson / KyleS.

La base histórica del proyecto expone interfaces de SteamWorks hacia SourcePawn
mediante una extensión nativa de SourceMod.

Este fork mantiene y adapta la extensión para el flujo de trabajo usado por
AoC-Gamers, principalmente en servidores dedicados Linux de 32 bits para
Left 4 Dead 2.

## Estado del proyecto

[P5-R19](docs/runtime-compatibility/p5-integration.md) integra las correcciones
GC/HTTP de revisión 12, conserva la API pública y repite la validación x86.
El nuevo ELF tiene aceptación SRCDS de HTTP real, GC real vacío y proveedor GC
local tipado. **Recepción positiva GC remota pendiente.** La matriz distingue
la aceptación actual de la evidencia histórica de P5-R18; no se declara cierre
global nuevo ni se despliega. Paquetes y reversión renovados localmente.

La evidencia histórica de [T25/T29](docs/runtime-compatibility/last-two-srcds-progress.md),
[T32](docs/runtime-compatibility/connection-srcds-acceptance.md) y
[T33](docs/runtime-compatibility/soak-srcds-acceptance.md) conserva sus hashes y
límites originales; no se presenta como repetida con el nuevo ELF.

Quince suites x86, API pública y sanitizadores pasan. Se recompilan consumidores
y 19 harnesses; las 66 pruebas Python y el build de Connect pasan. Los cuatro
paquetes P5-R19 están en `.build/p5-r19/review/`, con manifiesto, checksums y
reversión renovada. Los paquetes anteriores permanecen intactos. Source-review
incluye evidencia, recetas offline y el parche completo de Connect; los SMX HTTP
antiguos de tres argumentos requieren recompilación. La
[reversión](docs/runtime-compatibility/p5-rollback.md) conserva los bytes ZIP ya
ejercitados; el baseline mantiene su fallo conocido al salir. DEV original se
comprueba por lectura antes/después, sin operaciones de servidor en esta
actualización. Rama develop, sin commit, publicación ni despliegue.

El repositorio se mantiene principalmente para compilar y distribuir la extensión
Linux de 32 bits usada por servidores dedicados de L4D2.

El flujo principal de releases está orientado a Linux 32-bit. También existe
soporte de compilación Windows como flujo separado, pero el artefacto oficial
principal sigue siendo Linux.

Flujo recomendado en Linux:

```bash
make deps-exts-linux
make build-exts-linux STEAMWORKS_SDK_NAME=sdk_158a
make package-exts-linux
make release-linux RELEASE_BASENAME=steamworks-local-linux
```

Flujo recomendado en Windows:

```powershell
make deps-exts-windows
make build-exts-windows STEAMWORKS_SDK_NAME=sdk_158a
make package-exts-windows
make release-windows RELEASE_BASENAME=steamworks-local-windows
```

Artefacto Linux esperado:

```text
.build/linux-l4d2/package/addons/sourcemod/extensions/steamworks.ext.so
```

Artefacto Windows esperado:

```text
.build/windows-l4d2/package/addons/sourcemod/extensions/steamworks.ext.dll
```

## Qué hace técnicamente

SteamWorks carga una extensión nativa de SourceMod llamada:

```text
steamworks.ext
```

Durante la carga, la extensión registra la librería `SteamWorks` y crea módulos
internos para:

- acceso a `ISteamClient`;
- acceso a `ISteamGameServer`;
- acceso a `ISteamUtils`;
- acceso a `ISteamNetworking`;
- acceso a `ISteamGameServerStats`;
- acceso a `ISteamHTTP`;
- acceso a `ISteamMatchmaking`;
- acceso a `ISteamGameCoordinator`;
- natives de GameServer;
- natives de stats;
- natives HTTP;
- natives de Game Coordinator;
- forwards de eventos Steam;
- hooks y detours sobre funciones del gameserver.

La extensión no inicializa Steam desde cero como si fuera una aplicación de
escritorio. Se monta sobre el runtime Steam que ya usa el servidor dedicado.

## Arquitectura

El código nativo vive principalmente en:

```text
extension/
```

Estructura relevante:

```text
extension/
├── extension.cpp
├── extension.h
├── AMBuilder
├── core/
│   ├── swgameserver.cpp
│   ├── swgamedata.cpp
│   └── swforwards.cpp
├── natives/
│   ├── gsnatives.cpp
│   ├── ssnatives.cpp
│   └── gcnatives.cpp
├── hooks/
│   ├── swgshooks.cpp
│   ├── swgsdetours.cpp
│   └── swgchooks.cpp
├── http/
│   ├── swhttp.cpp
│   └── swhttprequest.cpp
├── steamtools/
├── CDetour/
├── asm/
└── sdk/
```

### `extension/core/`

Contiene la integración base con SteamWorks.

Responsabilidades principales:

- resolver `ISteamClient`;
- obtener interfaces del gameserver;
- cargar `gamedata/steamworks.txt` si existe;
- crear forwards globales hacia SourcePawn;
- resolver versiones de interfaces configurables por gamedata;
- manejar callbacks como conexión/desconexión de Steam, validación de clientes y estado de grupos.

### `extension/natives/`

Contiene los natives expuestos a SourcePawn.

Áreas principales:

- GameServer natives;
- stats natives;
- Game Coordinator natives.

### `extension/http/`

Contiene la implementación de requests HTTP mediante `ISteamHTTP`.

Incluye:

- creación de handles SourceMod para requests;
- callbacks de request completada;
- callbacks de headers recibidos;
- callbacks de datos recibidos por streaming;
- lectura de headers;
- lectura de body;
- escritura de body a archivo;
- helpers para body textual.

### `extension/hooks/`

Contiene hooks y detours sobre interfaces del gameserver y Game Coordinator.

Incluye:

- hook de `ISteamGameServer::WasRestartRequested`;
- hook de `ISteamGameServer::LogOnAnonymous`;
- hook de `ISteamGameServer::BeginAuthSession`;
- detour sobre `SteamGameServer_InitSafe`;
- detour sobre `SteamGameServer_Shutdown`;
- hooks sobre `ISteamGameCoordinator`.

### `scripting/include/`

Contiene la API pública SourcePawn:

```text
scripting/include/steamworks.inc
```

Este include es el punto de entrada para plugins que quieran usar la extensión.

## SDK de SteamWorks

La versión objetivo usada por este proyecto es:

```text
Steamworks SDK 1.58a
```

Importante:

- `make deps-exts-linux` no descarga el Steamworks SDK;
- `make deps-exts-windows` no descarga el Steamworks SDK;
- el SDK debe colocarse manualmente dentro del repositorio o apuntarse con una variable de entorno.

Directorios locales válidos:

```text
sdk/
sdk_155/
sdk_158a/
```

Por defecto, el build usa:

```text
STEAMWORKS_SDK_NAME=sdk
```

Ejemplo usando SDK 1.58a:

```bash
make build-exts-linux STEAMWORKS_SDK_NAME=sdk_158a
```

También puedes apuntar directamente a una ruta:

```bash
make build-exts-linux STEAMWORKS_SDK_DIR=/ruta/al/steamworks-sdk
```

En CI, el SDK se obtiene desde:

```text
AoC-Gamers/Steamworks-SDK
```

y se usa el directorio:

```text
sdk_158a
```

## Gamedata

La extensión puede usar gamedata para seleccionar versiones admitidas de interfaces
y ajustar la ruta de biblioteca. Las versiones desconocidas se rechazan
antes de adquirir la interfaz; no hay conversión entre ABI históricas. El registro
y los límites de esta validación se describen en [P2](docs/runtime-compatibility/p2-typed-interfaces.md).

Ruta esperada:

```text
addons/sourcemod/gamedata/steamworks.txt
```

El gamedata es opcional: el repositorio actual no incluye un archivo
`steamworks.txt` ni lo incorpora a los paquetes. Si el servidor necesita
overrides, el archivo debe proporcionarse por separado en la ruta indicada.
Sin él, la extensión usa las versiones de interfaces y las rutas predeterminadas
del código y del SDK.

Este archivo puede definir, entre otros:

- versión de `ISteamGameServer`;
- versión de `ISteamUtils`;
- versión de `ISteamNetworking`;
- versión de `ISteamGameServerStats`;
- versión de `ISteamHTTP`;
- versión de `ISteamMatchmaking`;
- versión de `ISteamGameCoordinator`;
- ruta de `libsteam_api.so` o `steam_api.dll`.

Las entradas de lifecycle y el factory usan exports declarados; sus direcciones
no se sustituyen mediante firmas de gamedata.

Si el runtime de Steam cambia o el binario del servidor expone versiones
distintas de interfaces, deben revisarse este archivo, el registro y las
declaraciones compiladas. Cambiar una cadena no habilita una ABI no admitida.

La carga requiere preparar los observadores de inicio/shutdown para retirar los
recursos antes de invalidar interfaces. Si falla esa preparación o la creación
del tipo de handle HTTP, se revierte la carga con un diagnóstico. Propiedad,
orden de desmontaje y pruebas: [P3](docs/runtime-compatibility/p3-resource-lifecycle.md).

**Perfil P4:** se validó localmente la pareja de binarios Linux x86 de
`competitive_dev` (juego 2.2.4.3, build 23990100). El selector exige sus build-id,
exports/RVA, el `CALL` de init del motor y el prefijo de shutdown; versiones,
módulos ambiguos, cambios en esos puntos y plataformas sin perfil se rechazan.
La [corrección de convivencia](docs/runtime-compatibility/t08-init-coexistence.md)
conserva el hook compartido de SourceTVSupport en el SDK y observa el retorno
desde el llamador. Las pruebas x86 y la [aceptación real de T08](docs/runtime-compatibility/t08-coexist-srcds-acceptance.md)
pasan para el ELF aceptado; véase la matriz P5 actual y sus límites de perfil.
Los manifiestos JSON documentan la evidencia y no activan código. Windows aún
no tiene un perfil validado. Detalles: [P4](docs/runtime-compatibility/p4-binary-profiles.md).

## API SourcePawn

La API pública está en:

```text
scripting/include/steamworks.inc
```

### Estado base del servidor

```sourcepawn
native bool SteamWorks_IsVACEnabled();
native bool SteamWorks_GetPublicIP(int ipaddr[4]);
native int SteamWorks_GetPublicIPCell();
native bool SteamWorks_IsLoaded();
native bool SteamWorks_IsConnected();
```

Estas funciones permiten consultar si la extensión está operativa, si el servidor
está conectado a Steam, si VAC está activo y cuál es la IP pública reportada por
Steam.

### Metadata del servidor

```sourcepawn
native bool SteamWorks_SetGameData(const char[] data);
native bool SteamWorks_SetGameDescription(const char[] description);
native bool SteamWorks_SetMapName(const char[] mapName);
native bool SteamWorks_SetRule(const char[] key, const char[] value);
native bool SteamWorks_ClearRules();
```

Estas funciones modifican información visible o consultable del servidor, como
descripción, mapa y reglas personalizadas.

### Identidad, grupos y licencias

```sourcepawn
native bool SteamWorks_GetUserGroupStatus(int client, int groupid);
native bool SteamWorks_GetUserGroupStatusAuthID(int authid, int groupid);

native EUserHasLicenseForAppResult SteamWorks_HasLicenseForApp(int client, int app);
native EUserHasLicenseForAppResult SteamWorks_HasLicenseForAppId(int authid, int app);

native int SteamWorks_GetClientSteamID(int client, char[] steamId, int length);
```

Estas funciones permiten consultar membresía de grupos Steam, licencias de apps
y SteamID64 de jugadores.

`SteamWorks_GetClientSteamID` conserva su conteo histórico: con capacidad positiva
devuelve bytes almacenados incluyendo `\0`, también si el texto se trunca.
Con capacidad cero devuelve 0 sin escribir; una capacidad negativa genera error
nativo. Ver [contrato y pruebas de conteo](docs/runtime-compatibility/p1-e-steamid-count.md).

Para L4D2, el AppID es:

```text
550
```

### Stats de usuarios

```sourcepawn
native bool SteamWorks_RequestStatsAuthID(int authid, int appid);
native bool SteamWorks_RequestStats(int client, int appid);

native bool SteamWorks_GetStatCell(int client, const char[] key, int &value);
native bool SteamWorks_GetStatAuthIDCell(int authid, const char[] key, int &value);

native bool SteamWorks_GetStatFloat(int client, const char[] key, float &value);
native bool SteamWorks_GetStatAuthIDFloat(int authid, const char[] key, float &value);
```

El flujo correcto es:

1. Solicitar stats con `SteamWorks_RequestStats` o `SteamWorks_RequestStatsAuthID`.
2. Esperar a que Steam tenga los datos disponibles.
3. Leer la key exacta con `SteamWorks_GetStatCell` o `SteamWorks_GetStatFloat`.

Las keys no son los nombres visibles de Steam. Deben obtenerse desde el schema
del juego.

Para L4D2, revisar:

```text
docs/GetSchemaForGame_550.md
docs/GetSchemaForGame_550.json
```

Ejemplo de key validada:

```text
Stat.GamesPlayed.Total
```

### HTTP

La extensión expone una capa HTTP basada en `ISteamHTTP`.

Creación y configuración:

```sourcepawn
native Handle SteamWorks_CreateHTTPRequest(EHTTPMethod method, const char[] url);
native bool SteamWorks_SetHTTPRequestContextValue(Handle request, any data1, any data2=0);
native bool SteamWorks_SetHTTPRequestNetworkActivityTimeout(Handle request, int timeout);
native bool SteamWorks_SetHTTPRequestHeaderValue(Handle request, const char[] name, const char[] value);
native bool SteamWorks_SetHTTPRequestGetOrPostParameter(Handle request, const char[] name, const char[] value);
native bool SteamWorks_SetHTTPRequestUserAgentInfo(Handle request, const char[] userAgentInfo);
native bool SteamWorks_SetHTTPRequestRequiresVerifiedCertificate(Handle request, bool requireVerifiedCertificate);
native bool SteamWorks_SetHTTPRequestAbsoluteTimeoutMS(Handle request, int timeoutMs);
```

Envío:

```sourcepawn
native bool SteamWorks_SetHTTPCallbacks(
    Handle request,
    SteamWorksHTTPRequestCompleted onCompleted = INVALID_FUNCTION,
    SteamWorksHTTPHeadersReceived onHeaders = INVALID_FUNCTION,
    SteamWorksHTTPDataReceived onData = INVALID_FUNCTION,
    Handle caller = null
);

native bool SteamWorks_SendHTTPRequest(Handle request);
native bool SteamWorks_SendHTTPRequestAndStreamResponse(Handle request);
native bool SteamWorks_DeferHTTPRequest(Handle request);
native bool SteamWorks_PrioritizeHTTPRequest(Handle request);
```

Lectura de respuesta:

```sourcepawn
native bool SteamWorks_GetHTTPResponseHeaderSize(Handle request, const char[] header, int &size);
native bool SteamWorks_GetHTTPResponseHeaderValue(Handle request, const char[] header, char[] value, int size);
native bool SteamWorks_GetHTTPResponseBodySize(Handle request, int &size);
native bool SteamWorks_GetHTTPResponseBodyData(Handle request, char[] body, int length);
native bool SteamWorks_GetHTTPResponseBodyString(Handle request, char[] body, int length, int &written, bool &truncated);
native bool SteamWorks_GetHTTPStreamingResponseBodyData(Handle request, int offset, char[] body, int length);
native bool SteamWorks_GetHTTPDownloadProgressPct(Handle request, float &percent);
native bool SteamWorks_GetHTTPRequestWasTimedOut(Handle request, bool &wasTimedOut);
```

Body y archivos:

```sourcepawn
native bool SteamWorks_SetHTTPRequestRawPostBody(Handle request, const char[] contentType, const char[] body, int bodyLength);
native bool SteamWorks_SetHTTPRequestRawPostBodyFromFile(Handle request, const char[] contentType, const char[] fileName);
native bool SteamWorks_GetHTTPResponseBodyCallback(Handle request, SteamWorksHTTPBodyCallback callback, any data = 0, Handle plugin = null);
native bool SteamWorks_WriteHTTPResponseBodyToFile(Handle request, const char[] fileName);
```

#### Nota importante sobre body HTTP

`SteamWorks_GetHTTPResponseBodyData` requiere el tamaño completo obtenido con
`SteamWorks_GetHTTPResponseBodySize`; no permite una lectura parcial. Lee bytes
y no garantiza que el buffer quede terminado en `\0`.

Para texto, JSON o HTML, usar preferentemente:

```sourcepawn
SteamWorks_GetHTTPResponseBodyString(...)
```

o reservar espacio para el cuerpo completo y cerrar manualmente el buffer
después de una lectura raw exitosa.

Ejemplo conceptual:

```sourcepawn
char body[2048];
int written;
bool truncated;

if (SteamWorks_GetHTTPResponseBodyString(request, body, sizeof(body), written, truncated))
{
    PrintToServer("written=%d truncated=%d body=%s", written, truncated, body);
}
```

### Game Coordinator

La extensión incluye soporte básico para Game Coordinator:

```sourcepawn
native EGCResults SteamWorks_SendMessageToGC(int messageType, const char[] data, int dataLength);
```

También existen forwards para observar/interceptar mensajes:

```sourcepawn
forward EGCResults SteamWorks_GCSendMessage(int messageType, const char[] data, int dataLength);
forward void SteamWorks_GCMsgAvailable(int dataLength);
forward EGCResults SteamWorks_GCRetrieveMessage(int messageType, const char[] destination, int destinationLength, int messageSize);
```

Esta área existe en código, pero la cobertura práctica del repositorio indica
que no tiene todavía un harness dedicado de validación runtime para L4D2.

## Forwards

### Validación de cliente

```sourcepawn
forward void SW_OnValidateClient(int ownerauthid, int authid);
forward void SteamWorks_OnValidateClient(int ownerauthid, int authid);
```

`SW_OnValidateClient` es el forward legacy.

`SteamWorks_OnValidateClient` es el forward recomendado.

Estos forwards permiten detectar casos donde el dueño real del juego no coincide
con el jugador conectado, por ejemplo Family Sharing.

### Conexión del servidor a Steam

```sourcepawn
forward void SteamWorks_SteamServersConnected();
forward void SteamWorks_SteamServersConnectFailure(EResult result);
forward void SteamWorks_SteamServersDisconnected(EResult result);
```

Permiten reaccionar a conexión, falla o desconexión del servidor frente a Steam.

### Restart solicitado por Steam

```sourcepawn
forward Action SteamWorks_RestartRequested();
```

Permite interceptar una solicitud de restart desde Steam.

### Token solicitado

```sourcepawn
forward void SteamWorks_TokenRequested(char[] token, int maxlen);
```

Permite entregar token en juegos que lo requieran.

L4D2 normalmente no lo necesita, pero el forward existe para compatibilidad con
otros runtimes Source.

### Estado de grupo Steam

```sourcepawn
forward void SteamWorks_OnClientGroupStatus(int authid, int groupid, bool isMember, bool isOfficer);
```

Se dispara cuando Steam resuelve si un usuario pertenece a un grupo.

## Interfaces soportadas y limitaciones

### Soportado / validado

Según las pruebas documentadas del repositorio, se verificaron en runtime:

- estado base de la extensión;
- conexión con Steam;
- VAC;
- public IP;
- metadata del servidor;
- reglas personalizadas;
- identidad de cliente;
- grupos;
- licencias;
- requests HTTP básicos;
- lectura de body HTTP;
- stats de usuario con keys reales de L4D2;
- algunos forwards como `SteamWorks_SteamServersConnected` y `SteamWorks_OnClientGroupStatus`.

### Parcialmente verificado

Existen funciones implementadas pero no todas fueron ejercitadas una por una con
casos dedicados:

- helpers avanzados de HTTP;
- streaming HTTP completo;
- escritura de body a archivo;
- timeouts específicos;
- requests diferidos/priorizados;
- algunos forwards de error/desconexión/restart.

### No verificado completamente

Game Coordinator existe en la extensión, pero no hay todavía una validación
práctica completa documentada para L4D2.

### No soportado: `ISteamFriends`

`ISteamFriends` fue investigado específicamente en el runtime Linux del servidor
dedicado de L4D2 con Steamworks SDK `1.58a`.

Conclusión:

- los símbolos existen en `libsteam_api.so`;
- `SteamFriends017` existe como string/símbolo;
- pero `GetISteamFriends(...)` devuelve `NULL`;
- el fallback `SteamAPI_SteamFriends_v017()` también devuelve `NULL`;
- por lo tanto, esta extensión no expone soporte para `ISteamFriends`.

Si se necesitan datos de perfil, persona o amigos, la ruta recomendable es usar
Web API/HTTP externo, no `ISteamFriends` local desde el gameserver.

Ver:

```text
docs/ISteamFriends.md
```

## Requisitos

### Runtime del servidor

Para cargar la extensión en el gameserver:

- servidor dedicado de Left 4 Dead 2 en Linux;
- SourceMod Linux x86;
- MetaMod:Source Linux x86;
- `bin/libsteam_api.so` disponible en la instalación del servidor;
- binarios de 32 bits funcionales en el sistema.

La extensión Linux se enlaza contra:

```text
libsteam_api.so
```

y se genera con `rpath` hacia:

```text
$ORIGIN/../../../../bin
```

Eso significa que, cargada desde:

```text
addons/sourcemod/extensions/steamworks.ext.so
```

intentará resolver la librería Steam desde:

```text
bin/libsteam_api.so
```

relativo a la raíz del juego.

En una instalación típica de L4D2 dedicada, esto corresponde a:

```text
left4dead2/bin/libsteam_api.so
```

### Build Linux

Para compilar en Linux necesitas:

- `bash`;
- `git`;
- `make`;
- `python3`;
- `python3-venv`;
- compilador con soporte 32-bit;
- librerías multilib;
- Steamworks SDK disponible localmente.

En Ubuntu/Debian:

```bash
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install -y python3 python3-venv make gcc-multilib g++-multilib clang zip
```

### Build Windows

Para compilar en Windows necesitas:

- `git`;
- `make`;
- `PowerShell 7+`;
- `python` 3.x;
- Visual Studio Build Tools o Visual Studio con soporte C++;
- toolchain MSVC x86/x64;
- Steamworks SDK disponible localmente.

Si `cl.exe` no está disponible, el script intenta ubicar Visual Studio mediante
`vswhere.exe` y cargar `vcvarsall.bat`.

## Compilación local

### Linux

Preparar dependencias:

```bash
make deps-exts-linux
```

Compilar usando SDK por defecto `sdk/`:

```bash
make build-exts-linux
```

Compilar usando SDK `sdk_158a/`:

```bash
make build-exts-linux STEAMWORKS_SDK_NAME=sdk_158a
```

Artefactos:

```text
.build/linux-l4d2/package/addons/sourcemod/extensions/steamworks.ext.so
.build/linux-l4d2/package/addons/sourcemod/scripting/include/steamworks.inc
```

### Windows

Preparar dependencias:

```powershell
make deps-exts-windows
```

Compilar:

```powershell
make build-exts-windows STEAMWORKS_SDK_NAME=sdk_158a
```

Artefactos:

```text
.build/windows-l4d2/package/addons/sourcemod/extensions/steamworks.ext.dll
.build/windows-l4d2/package/addons/sourcemod/scripting/include/steamworks.inc
```

## Targets disponibles

```text
make help
make deps-exts-linux
make deps-exts-windows
make build-exts-linux
make build-exts-windows
make package-exts-linux
make package-exts-windows
make release-linux
make release-windows
make clean-linux
make clean-windows
make clean-all
```

## Variables de entorno

Los scripts permiten sobreescribir rutas locales sin editar el repositorio.

Variables relevantes:

```text
DEPS_DIR
BUILD_DIR
HL2SDK_DIR
SOURCEMOD_DIR
MMSOURCE_DIR
STEAMWORKS_SDK_DIR
STEAMWORKS_SDK_NAME
VENV_DIR
TARGET_SDK
ENV_FILE
```

Al usar `make`, las rutas predeterminadas son relativas a la raíz del repo:

| Ruta | Linux | Windows |
| --- | --- | --- |
| Dependencias | `.deps/exts-linux` | `.deps/exts-windows` |
| Build AMBuild | `.build/linux-l4d2` | `.build/windows-l4d2` |
| Paquete preparado | `.build/package-exts-linux` | `.build/package-exts-windows` |
| Artefacto de release | `dist/linux/artifact` | `dist/windows/artifact` |

El SDK usa `STEAMWORKS_SDK_NAME=sdk` y se busca en
`<repo>/<STEAMWORKS_SDK_NAME>`, salvo que se indique `STEAMWORKS_SDK_DIR`.
El script Linux usa `TARGET_SDK=l4d2`; el script Windows también compila para L4D2.

Para cambiar las rutas desde `make`, usar sus variables por plataforma, por
ejemplo:

```bash
make deps-exts-linux LINUX_DEPS_DIR=/ruta/dependencias-linux
make build-exts-linux LINUX_DEPS_DIR=/ruta/dependencias-linux LINUX_BUILD_DIR=/ruta/build-linux STEAMWORKS_SDK_NAME=sdk_158a
```

Los equivalentes Windows son `WINDOWS_DEPS_DIR` y `WINDOWS_BUILD_DIR`.
`make` pasa esas rutas a los scripts como `DEPS_DIR` y `BUILD_DIR`. Si los scripts
se ejecutan directamente, usan `<repo>/.deps` por defecto para las dependencias;
para reproducir la separación por plataforma hay que indicar `DEPS_DIR`.
`ENV_FILE` y `TARGET_SDK` son opciones de los scripts Linux; los scripts Windows
leen `.env` desde la raíz del repositorio. Las variables ya definidas en el
proceso, incluidas las que pasa `make`, tienen prioridad sobre `.env`.

También se puede crear un archivo local:

```text
.env
```

a partir de:

```text
.env.example
```

## Dependencias resueltas por script

Los scripts de dependencias descargan y preparan:

- `alliedmodders/hl2sdk`, rama `l4d2`;
- `alliedmodders/sourcemod`, rama `1.12-dev`;
- `alliedmodders/metamod-source`, rama `1.12-dev`;
- `alliedmodders/ambuild`.

Al usar los targets de `make`, las dependencias se instalan en
`.deps/exts-linux/` o `.deps/exts-windows/`, según la plataforma.

Dentro de cada directorio se crea un entorno virtual de Python para AMBuild:
`.venv-linux/` o `.venv-windows/`, respectivamente.

El Steamworks SDK no se descarga automáticamente.

## Contenido empaquetado

El paquete final generado por AMBuild incluye:

```text
addons/sourcemod/extensions/steamworks.ext.so
addons/sourcemod/scripting/include/steamworks.inc
```

En Windows:

```text
addons/sourcemod/extensions/steamworks.ext.dll
addons/sourcemod/scripting/include/steamworks.inc
```

No se empaquetan plugins de prueba dentro del artefacto final.

## Instalación en servidor

Copiar el contenido de:

```text
.build/linux-l4d2/package/
```

sobre la raíz del servidor donde vive `addons/sourcemod`.

El archivo principal debe quedar en:

```text
addons/sourcemod/extensions/steamworks.ext.so
```

El include queda disponible para compilar plugins en:

```text
addons/sourcemod/scripting/include/steamworks.inc
```

Además, el servidor debe tener disponible:

```text
bin/libsteam_api.so
```

Si `libsteam_api.so` no existe, no es de 32 bits o no puede cargarse, la
extensión no iniciará correctamente.

## Flujo de CI

El workflow principal usa jobs separados para:

- preparar dependencias;
- compilar Linux 32-bit;
- compilar Windows x86;
- preparar y validar los paquetes Linux y Windows;
- publicar releases con los ZIP de ambas plataformas.

En CI se clona también:

```text
AoC-Gamers/Steamworks-SDK
```

y se usa:

```text
sdk_158a
```

Linux 32-bit sigue siendo la plataforma principal. El job `publish` espera
los builds y paquetes de ambas plataformas. Los pushes a `main` y `develop`
actualizan los canales `channel/latest` y `channel/develop`, respectivamente;
los tags `v*` generan releases versionados.

Para detalle del sistema de build y del empaquetado actual, ver también:

```text
docs/BUILD_SYSTEM.md
docs/DEVELOPMENT.md
```

## Pruebas runtime

El repositorio documenta pruebas usando:

```text
scripting/steamwork_test.sp
```

Cobertura documentada:

```text
docs/TestCoverage.md
```

Las listas siguientes corresponden al plugin de ejemplo histórico. La cobertura
archivada de GC local, streaming, forwards y recuperación Steam está en la
[matriz histórica P5-R18](docs/runtime-compatibility/p5/history/p5-r18-before-review12-integration-validation-results.json); para los
buffers del ELF modificado, consultar la [aceptación posterior](docs/runtime-compatibility/review12-srcds-acceptance.md).
Los límites del ejemplo no son casos pendientes del candidato.

Áreas verificadas:

- estado base;
- VAC;
- public IP;
- conexión Steam;
- metadata;
- reglas;
- identidad;
- grupos;
- licencias;
- HTTP básico;
- stats con keys reales de L4D2;
- algunos forwards.

Áreas parcialmente verificadas:

- helpers avanzados HTTP;
- streaming;
- escritura a archivo;
- algunos forwards de error/restart/desconexión.

Área pendiente:

- Game Coordinator.

Área descartada:

- `ISteamFriends`.

## Documentación técnica adicional

Documentos disponibles:

- [Índice técnico](docs/README.md)
- [Desarrollo local](docs/DEVELOPMENT.md)
- [Sistema de build](docs/BUILD_SYSTEM.md)
- [Schema de L4D2](docs/GetSchemaForGame_550.md)
- [HTTP](docs/ISteamHTTP.md)
- [Limitaciones de ISteamFriends](docs/ISteamFriends.md)
- [Stats de usuarios](docs/ISteamUserStats.md)
- [Cobertura de pruebas](docs/TestCoverage.md)

Notas recomendadas:

- usar enlaces relativos dentro de `docs/README.md`;
- evitar rutas absolutas de una máquina local;
- mantener explícitas las limitaciones runtime para no reintroducir interfaces ya descartadas.

## Consideraciones importantes

Esta extensión depende del runtime Steam disponible dentro del servidor dedicado.
No todas las interfaces del SDK son utilizables en ese contexto.

Que una interfaz exista en headers o símbolos no significa que pueda resolverse
correctamente desde un gameserver.

`ISteamHTTP` está validado como usable.

`ISteamFriends` no debe tratarse como soportado en este fork.

Game Coordinator existe, pero requiere pruebas específicas antes de considerarse
estable para uso operativo.

La build Linux es la prioridad del proyecto.

El SDK de SteamWorks debe gestionarse manualmente por razones de licencia y
distribución.

## Licencia

Este proyecto deriva de SourcePawn SteamWorks y conserva su base GPL.

Revisar los archivos fuente y la licencia del repositorio antes de redistribuir
binarios modificados.
