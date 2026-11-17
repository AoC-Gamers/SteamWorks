# Identidad de clientes

No confundir índice de cliente, referencia de entidad, AccountID, AppID y SteamID64. El índice
selecciona un humano conectado y autorizado; bots, clientes fuera de rango o sin
autorización se rechazan según el native. AccountID mantiene los 32 bits de su
celda. SteamID64 tiene universo, tipo e instancia y debe ser individual y coherente
con la identidad validada de SourceMod.

Los argumentos `client` reciben índices entre 1 y `GetMaxClients()`, no referencias
de entidad. La consulta del slot usa `GetGamePlayer()` y exige conexión; no depende
de que exista una entidad del motor. Los índices inválidos se rechazan antes de
consultar el slot. Se conservan las comprobaciones de identidad y autorización
de cada native.

En solicitudes de stats se usa SteamID64 validado. Si falta únicamente ese texto
y existe AccountID validado, se construye identidad individual pública. El fallback
no rescata una identidad inválida o inconsistente. La variante AuthID recibe un
AccountID de una celda y usa universo público; no puede detectar que el plugin
truncó previamente un SteamID64 hasta una coincidencia válida de 32 bits.

## Salida textual

`SteamWorks_GetClientSteamID` conserva el contrato histórico: con capacidad
positiva devuelve bytes escritos **incluido el NUL**. Con capacidad uno produce
solo NUL y devuelve uno; con cero no escribe y devuelve cero. Truncamiento,
longitudes negativas y errores siguen sus comprobaciones nativas. No usar ese
conteo como longitud textual sin descontar el terminador cuando corresponda.
Otros helpers, como HTTP `written`, tienen contratos diferentes.

El plugin debe proporcionar memoria del tamaño anunciado; una conversión válida
de dirección base no acredita el buffer entero. Las regresiones de texto y stats
se encuentran en `test-steamid-string.sh` y `test-stats-natives.sh`; los negativos
de preautorización y el contraste de consumidores tienen harnesses separados.
La pertenencia a un grupo o un perfil Web API son capacidades distintas de la
identidad conectada y no se infieren de un AccountID válido.
