# Cobertura de pruebas de `steamworks.inc`

La [matriz P5-R20](runtime-compatibility/p5/validation-results.json) integra
T26/T35 de R19 y separa los ensayos históricos del ELF `72664c65…`.
**GC remoto positivo sigue pendiente; no se infiere aceptación global nueva.**

Se repiten y pasan 15 suites x86, API frente a P0, sanitizadores sin supresiones, 133 checks
GC instrumentados y 33 HTTP de proveedor estricto. El build optimizado reproduce
el binario de la [aceptación de revisión 12](runtime-compatibility/review12-srcds-acceptance.md):
372 checks en SRCDS, HTTP real, GC real vacío/local tipado, nuevo juego y quit
limpio. Los mensajes del proveedor local no son contenido del backend Steam.

Se recompilan tres consumidores, 19 harnesses, cuatro fixtures Connect y dos
controles de includes; 55 entradas SDK y el fixture de 733 claves permanecen
intactos. Las 66 pruebas Python y build de Connect pasan. Los ZIP se verifican
extraídos, se repite build/integración sin `.git` y el roundtrip rollback local.
Los sanitizadores y VM offline no certifican el proceso SRCDS completo.

Los 33 casos aprobados y tres no aplicables del checkpoint anterior se conservan
con su alcance y hash históricos. Los demás escenarios runtime no se repiten
ni se transfieren automáticamente al ELF nuevo. No se certifican Windows/x64
runtime, OOM real ni todos los métodos SDK. El CI previo de `adffed0` pasó siete
jobs; no se repite para este seguimiento. La regeneración inicial no operó
servidores. El
[seguimiento T26/T35](runtime-compatibility/p5-r19-rollback-srcds-acceptance.md)
acredita instalación del ZIP R19 local y reversión en el mismo árbol SRCDS
aislado, con observador mínimo y restauración DEV. R20 reproduce ese payload,
renueva los ZIP y repite el roundtrip local; no los instala en SRCDS ni opera
servidores. No se hace commit ni despliega; GC remoto sigue pendiente.

## Referencia histórica del plugin de ejemplo

Las secciones siguientes conservan observaciones previas de
`scripting/steamwork_test.sp` contra `scripting/include/steamworks.inc`. Describen
su ensayo original; la aceptación del candidato actual se limita a los harnesses,
hashes y resultados y procedencia de la matriz P5-R20. Los informes y paquetes anteriores se
conservan intactos y no se transfieren entre binarios distintos.

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

- [ISteamHTTP](ISteamHTTP.md)

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

- [ISteamUserStats](ISteamUserStats.md)
- [GetSchemaForGame_550](GetSchemaForGame_550.md)

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

- [ISteamFriends](ISteamFriends.md)
