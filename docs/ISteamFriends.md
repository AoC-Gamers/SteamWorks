# ISteamFriends

## Resumen

`ISteamFriends` está presente en el runtime Linux distribuido con el servidor dedicado de Left 4 Dead 2, pero no es utilizable desde esta extensión en el contexto del gameserver.

Esto fue validado contra el runtime usando `Steamworks SDK 1.58a`.

## Qué se probó

La extensión fue modificada para intentar ambas rutas comunes de resolución:

1. `ISteamClient::GetISteamFriends(hSteamUser, hSteamPipe, "SteamFriends017")`
2. `SteamAPI_SteamFriends_v017()`

El plugin de pruebas también ejercitó los flujos relacionados que serían necesarios para datos de persona y clanes.

## Evidencia en runtime

El runtime del servidor dedicado expone los símbolos y strings relevantes:

- `SteamFriends017`
- `SteamAPI_SteamFriends_v017`
- `SteamAPI_ISteamClient_GetISteamFriends`

Sin embargo, en runtime ambas rutas de resolución devolvieron `NULL`:

```text
GetFriends: GetISteamFriends returned (nil)
GetFriends: fallback SteamAPI_SteamFriends_v017 returned (nil)
```

El mismo proceso resolvió y utilizó correctamente otras interfaces, por ejemplo:

- `ISteamClient`
- `ISteamGameServer`
- `ISteamHTTP`

Eso significa que la falla es específica de `ISteamFriends`, no del bootstrap de la extensión ni de la inicialización general de Steam.

## Conclusión

Para el runtime de servidor dedicado de Left 4 Dead 2 usado por este proyecto:

- `ISteamFriends` existe dentro de las bibliotecas distribuidas
- pero no se inicializa de forma utilizable para el contexto del gameserver
- por lo tanto no debe exponerse como una capacidad soportada por esta extensión

## Decisión práctica

La extensión elimina el intento de integración con `ISteamFriends` y no expone natives ni forwards relacionados.

Si se necesitan datos de perfil o persona para lógica de juego o moderación, la ruta confiable es un flujo externo por HTTP/Web API y no la interfaz local `ISteamFriends` del gameserver.
