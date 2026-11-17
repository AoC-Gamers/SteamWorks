# Stats

`SteamWorks_RequestStats(client, appid)` y `SteamWorks_RequestStatsAuthID(authid,
appid)` conservan dos argumentos. `appid` debe ser no cero e igual a `GetAppID()`
del `ISteamUtils` del servidor. Si no coincide, falta una interfaz o el contexto
no es válido, devuelven false sin enviar `RequestUserStats`.

La variante cliente recibe un índice de slot válido, exige un humano
conectado/autorizado y su identidad validada; no requiere resolver una referencia
de entidad.
La variante AuthID recibe AccountID de 32 bits, no SteamID64, y construye identidad
individual pública. El segundo argumento no selecciona universo ni otro juego.
Un true indica solicitud asíncrona aceptada, no disponibilidad de los stats.
Espera `SteamWorks_OnUserStatsReceived(int authid, EResult result, int appid)` y
comprueba después las lecturas. Este forward informa las solicitudes hechas por
la extensión, con AccountID de 32 bits y el AppID capturado al solicitarlas;
`authid` no es un índice de cliente. Las solicitudes simultáneas de la misma
identidad y AppID comparten una operación pendiente y una notificación.

Cada solicitud usa su propio `CCallResult<GSStatsReceived_t>` marcado para el
servidor de juego. Una recepción broadcast de ese tipo no completa la solicitud.
El resultado conserva el `EResult` de Steam; un fallo de transporte devuelve
`k_EResultIOFailure` y una respuesta nula o con otra identidad devuelve
`k_EResultFail`. Incluso `k_EResultOK` requiere comprobar cada lectura: no garantiza
que todas las claves existan. La parada del contexto o la descarga de la extensión
cancela las operaciones pendientes sin emitir resultados posteriores.

`SteamWorks_OnUserStatsUnloaded(int authid)` informa que Steam ha descargado de
caché los stats de esa identidad. Solicítalos otra vez antes de leerlos. Ambos
forwards son notificaciones (`ET_Ignore`); no permiten alterar el resultado ni
escribir stats. No se añade un forward de almacenamiento, pues la API pública
actual no solicita escrituras.

En SDK 1.55, `GSStatsUnloaded_t` y el callback de cliente `UserStatsUnloaded_t`
comparten el ID `1108`; la suscripción GameServer distingue el origen. La entrega
simulada comprueba esa separación con ambos suscriptores registrados a la vez.

## Descarga de caché en L4D2 dedicado x86

El forward se conserva por compatibilidad, pero su emisión depende de que Steam
genere `GSStatsUnloaded_t` en el contexto GameServer. Valve
[documenta la notificación](https://partner.steamgames.com/doc/api/ISteamGameServerStats#GSStatsUnloaded_t)
y que los stats cargados por un servidor pueden caducar en
[Stats y logros](https://partner.steamgames.com/doc/features/achievements).
No publica una operación para forzar la descarga, capacidad de caché ni plazo
garantizado. La política LRU descrita para `ISteamUserStats` del cliente no
establece un disparador reproducible para el GameServer de L4D2.

No se garantiza el evento al desconectar, reconectar o cambiar de mapa. Los
consumidores no deben depender de su recepción para liberar sus propios recursos;
su ausencia durante una ventana de observación no prueba un fallo del forward.
La recepción real no está acreditada en L4D2 dedicado x86. Se mantienen las
pruebas deterministas de entrega, aislamiento de contextos y ciclo de vida.

## Lecturas y cobertura

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

La suite `make test-stats-callbacks` comprueba el propietario asíncrono con los
tipos reales del SDK y entrega controlada, ASan y UBSan x86. Incluye resultados
fuera de orden, errores, aislamiento de contextos, AccountID con bit alto,
reentrada, cancelación y desmontaje durante ambos forwards. El control negativo
sin filtro de contexto debe fallar en la comprobación de aislamiento.

Se realizaron pruebas locales en SRCDS Linux x86 de
`SteamWorks_OnUserStatsReceived`: `k_EResultOK`, identidad/AppID coincidentes y
lecturas enteras válidas por ambas variantes. Tres solicitudes simultáneas se
agruparon en un resultado y una solicitud posterior produjo otro. Un ensayo de
descarga/recarga con una solicitud pendiente verificó ausencia de callbacks
antiguos durante su ventana y nuevas lecturas correctas. Este último resultado
es acotado; no demuestra ausencia indefinida de callbacks.

El observador corregido conservó el objetivo tras finalizar y tras una
desconexión confirmada. Durante cinco minutos sin nuevas solicitudes no llegó
ningún `SteamWorks_OnUserStatsUnloaded`. Esto comprueba la retención del objetivo,
no una descarga de caché ni un plazo de emisión de Steam. La cobertura del forward
es determinista; este ensayo no acredita su recepción real en SRCDS.

La [matriz de cobertura](https://github.com/AoC-Gamers/SteamWorks/blob/develop/tests/COVERAGE.md)
identifica el hash y el alcance de cada ensayo. No se transfiere aceptación entre
binarios: la entrega de resultados y el ensayo de cancelación conservan sus
atribuciones respectivas.
