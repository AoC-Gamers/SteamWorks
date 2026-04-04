# ISteamUserStats

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

- [GetSchemaForGame_550.json](C:\GitHub\SteamWorks\docs\GetSchemaForGame_550.json)

Eso permite revisar las `stat_key` sin depender del comando de prueba ni de una key cargada en runtime.

## Objetivo práctico

Con esa respuesta puedes:

- ubicar las keys reales de stats del juego
- luego probarlas con:
  - `sm_swtest_stats <target> <stat_key>`
  - `sm_swtest_stats_candidates <target>`
