# Adaptador SteamGameServer014

L4D2 enlaza el hook `BeginAuthSession` a **SteamGameServer014**, no a la
declaración 015 del SDK 1.58a utilizada por otros natives. El prefijo auditado
014 procede del SDK 1.55: tiene 27 slots hasta `BeginAuthSession` inclusive,
con su prototipo real. No se declara que ese prefijo permita utilizar el resto
de la interfaz 014.

`extension/core/swgameauth014.h` define un tipo independiente;
`swinterfaces.h` lo registra con su versión exacta. La resolución de autenticación
obtiene ese tipo, y SourceHook instala el hook sobre ese objeto. No se hace cast
entre 014 y 015. Las firmas públicas SourcePawn permanecen iguales.

`tests/fixtures/gameserver014-prefix.json` conserva la declaración independiente.
Las pruebas verifican orden y prototipo, rechazan la conversión 014/015 y usan
un objeto 014 junto a uno 015: solo el primero debe disparar el hook. Un control
con el enlace equivocado debe fallar por ese comportamiento.

Esto valida la adaptación de ABI en x86, no la emisión de un ticket válido ni la
conexión de una cuenta por sí sola. Para runtime se conserva el alcance histórico
en la cobertura, con el hash exacto de la extensión ensayada.
