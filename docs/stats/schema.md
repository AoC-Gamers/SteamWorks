# Schema de stats

La Steam Web API `ISteamUserStats/GetSchemaForGame/v2` describe las keys internas
y tipos. Para L4D2 el AppID es 550. Consultar la API requiere una clave: no la
incluyas en logs, ejemplos reales ni paquetes. Un nombre visible como un contador
de partidas no permite deducir su key interna ni su tipo numérico.

La copia histórica `tests/fixtures/stats/GetSchemaForGame_550.json` es una entrada
reproducible para pruebas y para el generador
`tests/helpers/generate-stats-acceptance-schema.py`. Vive en las fuentes,
no en el paquete de producto; no se actualiza como resultado de una ejecución.
El generador selecciona claves de esa copia para un harness y escribe sus salidas
fuera de `docs/`. El schema no acredita que una cuenta tenga un valor disponible.

Para consultar un dato real distingue entero/float, espera que los stats estén
disponibles y comprueba el retorno del lector. Las claves inexistentes deben
conservar la salida, según el [contrato de stats](README.md).
