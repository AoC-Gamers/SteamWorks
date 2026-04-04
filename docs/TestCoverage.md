# Cobertura de pruebas de `steamworks.inc`

Este documento resume qué partes de la biblioteca fueron verificadas en runtime
contra un servidor dedicado de Left 4 Dead 2, qué partes quedaron solo
parcialmente verificadas, qué partes no se ejercitaron todavía y qué partes se
consideran no soportadas en este contexto.

La referencia de API evaluada es:

- `Pawn/includes/steamworks.inc`

El plugin usado para la validación práctica fue:

- `Pawn/steamwork_test.sp`

## Verificado

### Estado base y conectividad

- `SteamWorks_IsVACEnabled`
- `SteamWorks_GetPublicIP`
- `SteamWorks_GetPublicIPCell`
- `SteamWorks_IsLoaded`
- `SteamWorks_IsConnected`

Resultado:

- verificado en runtime
- `SteamWorks_SteamServersConnected()` observado en logs

### Metadata del servidor

- `SteamWorks_SetGameData`
- `SteamWorks_SetGameDescription`
- `SteamWorks_SetMapName`
- `SteamWorks_SetRule`
- `SteamWorks_ClearRules`

Resultado:

- verificado en runtime con `sm_swtest_servermeta`

### Identidad, grupos y licencias

- `SteamWorks_GetUserGroupStatus`
- `SteamWorks_GetUserGroupStatusAuthID`
- `SteamWorks_HasLicenseForApp`
- `SteamWorks_HasLicenseForAppId`
- `SteamWorks_GetClientSteamID`

Resultado:

- verificado en runtime con `sm_swtest_group`, `sm_swtest_license` y
  `sm_swtest_identity`

Notas:

- para licencias, el valor `0` corresponde a
  `k_EUserHasLicenseResultHasLicense`

### `ISteamHTTP`

- `SteamWorks_CreateHTTPRequest`
- `SteamWorks_SetHTTPRequestContextValue`
- `SteamWorks_SetHTTPRequestHeaderValue`
- `SteamWorks_SetHTTPCallbacks`
- `SteamWorks_SendHTTPRequest`
- `SteamWorks_GetHTTPResponseBodySize`
- `SteamWorks_GetHTTPResponseBodyData`
- `SteamWorks_GetHTTPStreamingResponseBodyData`

Resultado:

- verificado en runtime con `sm_swtest_http`
- confirmada la lectura correcta de respuestas cortas y de JSON
- confirmado que el body viene como bytes y no garantiza terminador nulo

Documentación relacionada:

- [ISteamHTTP](C:\GitHub\SteamWorks\docs\ISteamHTTP.md)

### `ISteamUserStats`

- `SteamWorks_RequestStatsAuthID`
- `SteamWorks_RequestStats`
- `SteamWorks_GetStatCell`
- `SteamWorks_GetStatAuthIDCell`
- `SteamWorks_GetStatFloat`
- `SteamWorks_GetStatAuthIDFloat`

Resultado:

- request de stats verificada en runtime
- lectura concreta verificada con una key real de L4D2:
  - `Stat.GamesPlayed.Total`
- confirmado que esa key se resuelve como `cell`, no como `float`

Documentación relacionada:

- [ISteamUserStats](C:\GitHub\SteamWorks\docs\ISteamUserStats.md)
- [GetSchemaForGame_550](C:\GitHub\SteamWorks\docs\GetSchemaForGame_550.md)

### Forwards observados

- `SteamWorks_SteamServersConnected`
- `SteamWorks_OnClientGroupStatus`

Resultado:

- observados en logs del servidor durante pruebas reales

## Parcialmente verificado

### Helpers adicionales de HTTP

- `SteamWorks_SetHTTPRequestNetworkActivityTimeout`
- `SteamWorks_SetHTTPRequestGetOrPostParameter`
- `SteamWorks_SetHTTPRequestUserAgentInfo`
- `SteamWorks_SetHTTPRequestRequiresVerifiedCertificate`
- `SteamWorks_SetHTTPRequestAbsoluteTimeoutMS`
- `SteamWorks_SendHTTPRequestAndStreamResponse`
- `SteamWorks_DeferHTTPRequest`
- `SteamWorks_PrioritizeHTTPRequest`
- `SteamWorks_GetHTTPResponseHeaderSize`
- `SteamWorks_GetHTTPResponseHeaderValue`
- `SteamWorks_GetHTTPDownloadProgressPct`
- `SteamWorks_GetHTTPRequestWasTimedOut`
- `SteamWorks_SetHTTPRequestRawPostBody`
- `SteamWorks_SetHTTPRequestRawPostBodyFromFile`
- `SteamWorks_GetHTTPResponseBodyCallback`
- `SteamWorks_WriteHTTPResponseBodyToFile`

Resultado:

- la base de `ISteamHTTP` está validada
- estas rutas no fueron ejercitadas una por una con casos dedicados

### Forwards no observados todavía

- `SteamWorks_SteamServersConnectFailure`
- `SteamWorks_SteamServersDisconnected`
- `SteamWorks_RestartRequested`
- `SteamWorks_TokenRequested`

Resultado:

- los handlers existen en el plugin de prueba
- no todos fueron observados en runtime durante esta tanda de pruebas

## No verificado

### Game Coordinator

- `SteamWorks_SendMessageToGC`
- `SteamWorks_GCSendMessage`
- `SteamWorks_GCMsgAvailable`
- `SteamWorks_GCRetrieveMessage`

Resultado:

- no se creó un harness específico para GC
- no hay validación práctica en este repositorio todavía

## No soportado

### `ISteamFriends`

La interfaz fue investigada específicamente para el contexto de servidor
dedicado de L4D2.

Resultado:

- `GetISteamFriends(..., "SteamFriends017")` devolvió `NULL`
- el fallback `SteamAPI_SteamFriends_v017()` también devolvió `NULL`
- aunque los símbolos existen en `libsteam_api.so`, la interfaz no quedó
  disponible para uso práctico en este contexto

Conclusión:

- `ISteamFriends` no debe reintroducirse como base funcional para este fork

Documentación relacionada:

- [ISteamFriends](C:\GitHub\SteamWorks\docs\ISteamFriends.md)
