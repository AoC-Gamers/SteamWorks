# Changelog

## [Unreleased]

### Añadido

- **Stats:** nuevos forwards `SteamWorks_OnUserStatsReceived` y
  `SteamWorks_OnUserStatsUnloaded`. En L4D2 dedicado no se ha observado la entrega
  real del segundo. Los plugins deben gestionar la liberación de sus recursos
  independientemente de ese aviso.
- **HTTP:** `SteamWorks_GetHTTPResponseBodyString` permite leer respuestas como
  texto terminado en NUL e informa los bytes escritos y si hubo truncamiento.

### Cambiado

- **Compilación:** Steamworks SDK 1.55 (`sdk_155`) es la única versión admitida.
  La plataforma de ejecución validada es L4D2 dedicado Linux x86. Se ofrece un
  paquete para Windows, cuyo funcionamiento en el juego queda sin verificar.
- **Distribución:** sustituye el binario genérico anterior por
  `steamworks.ext.2.l4d2.so` o `steamworks.ext.2.l4d2.dll`, según la plataforma.
  Se conserva el nombre lógico `steamworks.ext`. Cada ZIP incluye el include
  público, `README.md`, `LICENSE` y `CHANGELOG.md`.
- **Releases:** Latest y Develop conservan sus enlaces de descarga. Las notas
  muestran la fecha de actualización en UTC, el commit y la ejecución de CI.

### Corregido

- **Stats:** los lectores conservan sus parámetros de salida si Steam devuelve
  un error. `SteamWorks_RequestStats*` usa el AppID real del juego. Los plugins
  que pasaban un universo en ese argumento deben sustituirlo por el AppID.
- **Identidad:** los natives de cliente validan el slot incluso cuando todavía
  no tiene entidad. `GetClientSteamID` conserva el conteo que incluye el
  terminador para buffers de capacidad positiva y devuelve cero si la capacidad
  es cero.
- **Autenticación:** el enlace de `BeginAuthSession` usa la interfaz
  `SteamGameServer014` y el forward conserva los bytes del ticket y su longitud.
- **Servidor:** la consulta de IP pública rechaza direcciones que no son IPv4 o
  que todavía no están disponibles.
- **HTTP:** los callbacks de streaming reciben el contexto completo de 64 bits
  y solo se registran cuando el envío tiene éxito. La lectura textual conserva
  las salidas si falla la copia del cuerpo.
- **Game Coordinator:** los forwards reciben únicamente bytes válidos, sin
  leer salidas que Steam no garantiza al fallar. La API se mantiene por
  compatibilidad y depende del soporte del juego y del contexto Steam.
- **Inicialización y cierre:** se evita la carga duplicada por nombre físico,
  se conservan los hooks de SourceTVSupport durante la reinicialización y se
  corrige un fallo al salir de SRCDS x86.
- **Gamedata:** los ajustes de interfaces de matchmaking y Game Coordinator
  se aplican al resolver sus versiones.

### Eliminado

- **API:** se retiran los natives y forwards de `ISteamFriends`, que no es
  utilizable en el contexto de L4D2 dedicado.

### Desarrollo

- **Análisis local:** Make ofrece análisis opcional con SonarQube y Cppcheck,
  además de cobertura de C++ x86 y Python. Sonar y la cobertura quedan fuera de CI.

## [1.2.4]

### Notas
- Baseline anterior del fork AoC antes del trabajo actual de modernización local.
