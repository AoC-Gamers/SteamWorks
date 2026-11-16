# Stats

`SteamWorks_RequestStats(client, appid)` y `SteamWorks_RequestStatsAuthID(authid,
appid)` conservan dos argumentos. `appid` debe ser no cero e igual a `GetAppID()`
del `ISteamUtils` del servidor. Si no coincide, falta una interfaz o el contexto
no es válido, devuelven false sin enviar `RequestUserStats`.

La variante cliente exige un humano conectado/autorizado y su identidad validada.
La variante AuthID recibe AccountID de 32 bits, no SteamID64, y construye identidad
individual pública. El segundo argumento no selecciona universo ni otro juego.
Un true indica solicitud asíncrona aceptada, no disponibilidad de los stats.
Espera el resultado correspondiente y comprueba después las lecturas.

Los lectores `SteamWorks_GetStatCell`, `GetStatAuthIDCell`, `GetStatFloat` y
`GetStatAuthIDFloat` **solo cambian la salida al devolver true**. Si Steam no
proporciona el dato, se conserva el valor previo; un cero válido también devuelve
true. Direcciones de nombre/salida inválidas generan error nativo antes de llamar
al proveedor. No uses el valor anterior como evidencia de una consulta nueva.

Las keys internas y sus tipos vienen del [schema](schema.md), no del nombre
amigable de la UI. La cobertura distingue enteros, floats positivos y claves
inexistentes, y solicitud aceptada frente a lectura real. Implementación:
`extension/natives/ssnatives.cpp`; regresiones: `stats_natives_test.cpp` y
`stats_requests_test.cpp`, ejecutadas por `test-stats-natives.sh`.
