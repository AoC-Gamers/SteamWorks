# ISteamUserStats

Los contratos corregidos tienen pruebas con proveedores simulados y aceptación
en SRCDS x86 del payload final: [stats enteros](runtime-compatibility/p5-r3-auth-srcds-acceptance.md)
y [12 claves float y clave inexistente](runtime-compatibility/p5-r4-stats-srcds-acceptance.md)
se validaron por cliente y AccountID con un humano autorizado. Una solicitud
aceptada por sí sola no certifica datos disponibles: los harnesses esperan lecturas
reales y contrastan sus resultados. Véase el alcance y los pendientes en [P5](runtime-compatibility/p5-integration.md).

## Resumen

La extensión puede solicitar stats del juego para usuarios concretos usando los
natives de stats, pero para conocer las `stat_key` válidas conviene consultar
el schema oficial del juego.

La fuente oficial para eso es la Web API:

- `ISteamUserStats/GetSchemaForGame/v2`

## Por qué hace falta

La UI pública de Steam muestra nombres amigables como:

- `Games played`
- `Infected killed`
- `Finales survived`

pero los natives de la extensión requieren la key interna exacta del stat.

Esa key no se deduce automáticamente desde la página del perfil.

## Endpoint útil

Ejemplo para Left 4 Dead 2 (`appid=550`):

```text
https://api.steampowered.com/ISteamUserStats/GetSchemaForGame/v2/?key=TU_KEY&appid=550
```

## Referencia local en este repositorio

Este repositorio ya incluye una copia descargada del schema para `appid=550`:

- [GetSchemaForGame_550.json](GetSchemaForGame_550.json)

Eso permite revisar las `stat_key` sin depender del comando de prueba ni de una key cargada en runtime.

## Contrato de salida de los lectores

Los cuatro lectores `SteamWorks_GetStatCell`, `SteamWorks_GetStatAuthIDCell`,
`SteamWorks_GetStatFloat` y `SteamWorks_GetStatAuthIDFloat` solo modifican el
argumento de salida si devuelven true. Si la consulta falla, conservan el valor
anterior. Un valor cero válido también devuelve true. Los errores de conversión
de la dirección del nombre o de la salida generan un error nativo antes de
ejecutar `GetUserStat`. Ver [corrección y pruebas deterministas](runtime-compatibility/p1-a-stats.md).

## Solicitudes: AppID e identidad

`SteamWorks_RequestStats(client, appid)` y
`SteamWorks_RequestStatsAuthID(authid, appid)` conservan sus firmas de dos
argumentos. `appid` debe ser distinto de cero y coincidir con `GetAppID()` de
`ISteamUtils` del servidor. Si el AppID no coincide, es desconocido o las
interfaces necesarias no están disponibles, devuelven false sin llamar a
`RequestUserStats`. No permiten elegir arbitrariamente otro juego.

En la variante AuthID, `authid` es el AccountID de 32 bits, conservando todos sus
bits, no un SteamID64. Se construye una identidad individual de universo público.
Se corrigió el comentario antiguo del include que mencionaba SteamID64 para
ese argumento de una celda. Si el llamador ya truncó un
SteamID64 a 32 bits, el native no puede distinguirlo de un AccountID.

La variante cliente requiere un humano conectado y autorizado y usa la identidad
validada de SourceMod. Si falta únicamente el SteamID64 y hay AccountID validado,
usa ese AccountID en universo público; no recupera una identidad inválida o
inconsistente mediante ese fallback. Bots y clientes no autorizados devuelven false.

Un retorno true significa que Steam aceptó la solicitud asíncrona, no que los
stats estén listos. Los plugins que usaban el segundo argumento como universo
deben pasar ahora el AppID real del contexto (550 en L4D2). Ver
[compatibilidad y pruebas](runtime-compatibility/p1-b-stats-identity.md).

## Objetivo práctico

Con esa respuesta puedes:

- ubicar las keys reales de stats del juego
- luego probarlas con:
  - `sm_swtest_stats <target> <stat_key>`
