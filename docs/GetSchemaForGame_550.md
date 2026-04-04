# GetSchemaForGame_550

## Resumen

Este documento explica cómo usar:

- [GetSchemaForGame_550.json](C:\GitHub\SteamWorks\docs\GetSchemaForGame_550.json)

como referencia local para probar stats de Left 4 Dead 2 con:

- [steamwork_test.sp](C:\GitHub\SteamWorks\Pawn\steamwork_test.sp)

El archivo corresponde al schema oficial de Steam para `appid=550`.

## Qué contiene

El schema mezcla al menos dos tipos de entradas relevantes:

- `stats`
- `achievements`

Es importante no mezclarlas.

### Stats

Las entries de `stats` son las que sirven para:

- `SteamWorks_GetStatCell`
- `SteamWorks_GetStatAuthIDCell`
- `SteamWorks_GetStatFloat`
- `SteamWorks_GetStatAuthIDFloat`

Ejemplo válido de `stat_key`:

- `Stat.GamesPlayed.Total`

### Achievements

Las entries de `achievements` usan keys como:

- `ACH_HONK_A_CLOWNS_NOSE`

Esas keys no sirven para `GetStatCell` o `GetStatFloat`.

Si se pasan al comando de stats, la lectura fallará aunque la key exista dentro del schema.

## Cómo usarlo con steamwork_test

Comando:

```text
sm_swtest_stats <target> <stat_key>
```

Ejemplo real validado:

```text
sm_swtest_stats Test Stat.GamesPlayed.Total
```

Resultado esperado:

- `clientReq=1`
- `authReq=1`
- `clientCellOk=1`
- `authCellOk=1`

En la prueba validada:

- `Stat.GamesPlayed.Total = 259`

## Reglas prácticas

1. Si la key viene de `achievements`, no la uses en `sm_swtest_stats`
2. Si la key viene de `stats`, prueba primero como `cell` y `float`
3. Si `clientCellOk=1` o `authCellOk=1`, entonces la key es válida para lectura entera
4. Si solo `clientFloatOk=1` o `authFloatOk=1`, entonces el stat está modelado como `float`

## Ejemplo de diferencia

Achievement key:

- `ACH_HONK_A_CLOWNS_NOSE`

Resultado esperado en `sm_swtest_stats`:

- no lectura
- todos los `*Ok=0`

Stat key:

- `Stat.GamesPlayed.Total`

Resultado esperado en `sm_swtest_stats`:

- lectura válida
- `clientCellOk=1`
- `authCellOk=1`

## Objetivo de este archivo

Evitar dos errores comunes:

- probar achievements como si fueran stats
- adivinar keys desde la UI pública del perfil de Steam en vez de usar el schema real
