# Cobertura de pruebas de `steamworks.inc`

La [matriz P5-R17](runtime-compatibility/p5/validation-results.json) cierra los
33 casos aplicables a Linux x86; T13/T14/T34 no aplican al perfil.
[T25/T29](runtime-compatibility/last-two-srcds-progress.md) incorporan 18 checks
humanos/sociales, 104 deterministas de Connect en VM x86 y 66 pruebas Python.
Quince suites x86 repetidas pasan, incluidos los sanitizadores sin supresiones
y la comparación de API pública. ELF e include permanecen idénticos a R16.

Se conservan las aceptaciones anteriores por hash y alcance: T11 capacidades/GC
local; T23 forwards; T15 buffers; T16 carga tardía/recarga quiescente; T20 descarga
del propietario HTTP; autenticación/stats optimizados; T26/T35 instalación y
reversión; T32 recuperación Steam y T33 carga local medida durante veinte minutos.
El cierre no certifica todos los métodos SDK, GC remoto, varias horas de carga,
Windows/x64, otro proveedor de identidad o todo el negocio de los consumidores.
Los SMX HTTP antiguos con tres argumentos deben recompilarse; Connect corregido
pasó arranque frío/HTTP/ACK/cambio de mapa. ZIP R17 regenerados, R16 intactos; el ZIP de revisión incorpora evidencia y
parche externo. Se recompilan 18 harnesses runtime, cuatro fixtures Connect y
dos controles de includes; 55 entradas SDK y 152 fuentes comprobadas. Build
optimizado idéntico tras fijar la fecha del compilador; el primer build y su
diferencia solo de fecha/build-id quedan documentados. Roundtrip rollback local
y replay de diagnósticos extraídos pasan. Sin nueva operación SRCDS, commit ni producción.

## Referencia histórica del plugin de ejemplo

Las secciones siguientes conservan observaciones previas de
`scripting/steamwork_test.sp` contra `scripting/include/steamworks.inc`. Describen
su ensayo original; la aceptación del candidato actual se limita a los harnesses,
hashes y resultados de la matriz P5-R17. Los informes y paquetes anteriores se
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
