# Interfaces y capacidades

El registro nativo admite exclusivamente estas declaraciones. Un cambio requiere
una auditoría; no se elige automáticamente una versión próxima.

| Tipo | Versión admitida | Uso |
|---|---|---|
| ISteamClient | SteamClient020 | Resolver interfaces |
| ISteamGameServer | SteamGameServer015 | Operaciones nativas de servidor |
| ISteamGameServer014 | SteamGameServer014 | Adaptador independiente del hook de autenticación |
| ISteamUtils | SteamUtils010 | AppID, universo y utilidades |
| ISteamNetworking | SteamNetworking006 | Red legacy expuesta |
| ISteamGameServerStats | SteamGameServerStats001 | Solicitudes y lectores de stats |
| ISteamHTTP | STEAMHTTP_INTERFACE_VERSION003 | Requests HTTP |
| ISteamMatchmaking | SteamMatchMaking009 | Capacidad de matchmaking expuesta |
| ISteamGameCoordinator | SteamGameCoordinator001 | Envío y recepción GC |

Las capacidades opcionales pueden faltar aunque la extensión cargue. Cada native
conserva su resultado/error definido ante ausencia; no deducir soporte por tener
headers del SDK. `ISteamFriends` no está expuesta: la consulta fue nula en el
contexto dedicado documentado, lo que tampoco demuestra ausencia universal.

El perfil operativo de init/shutdown está en `extension/core/swlifecycleprofile.h`:
comprueba identidad de módulos, prefijos y trampolines. Los JSON de fixtures son
documentales y permanecen deshabilitados; no activan el producto. Un binario
desconocido falla cerrado. No convertir la biblioteca del SDK en evidencia de
un módulo realmente cargado por el motor. Ver [compatibilidad](compatibility.md).
