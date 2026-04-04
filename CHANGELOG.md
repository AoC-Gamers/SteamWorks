# Changelog

Todos los cambios importantes de este proyecto se documentan en este archivo.

## [Unreleased]

### Agregado
- Se agregó `Pawn/steamwork_test.sp` como plugin local de pruebas para validación agrupada de SteamWorks.
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
- Se rehízo el workflow de GitHub Actions para compilar desde `AoC-Gamers/Steamworks-SDK`.
- Se reorganizó CI en jobs separados para:
  - preparación de dependencias
  - compilación Linux
  - compilación Windows
  - publicación de releases
- Se limitó el artefacto oficial de release Linux a:
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
- Se tradujeron y normalizaron los comentarios y nombres de parámetros en `steamworks.inc`.

### Eliminado
- Se eliminó el workflow antiguo de Travis CI.
- Se eliminaron plugins de ejemplo heredados que ya no forman parte del paquete final:
  - `Pawn/swag.sp`
  - `Pawn/UpdateCheck.sp`
- Se eliminaron los natives, forwards e integración runtime no soportados de `ISteamFriends`, tras validar que la interfaz no es utilizable en el contexto del servidor dedicado de L4D2.
- Se eliminó la lógica temporal de prueba de schema/Web API del plugin local de test, después de mover ese uso a documentación estática.

## [1.2.4]

### Notas
- Baseline anterior del fork AoC antes del trabajo actual de modernización local.
