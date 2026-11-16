# Adaptador SteamGameServer014

L4D2 y el SDK 1.55 utilizan **SteamGameServer014**. El prefijo auditado del motor
tiene 27 slots hasta `BeginAuthSession` inclusive y conserva su tipo independiente
en `extension/core/swgameauth014.h`; no permite usar el resto de la interfaz por
analogía. El SDK completo declara `GetAuthSessionTicket` con tres argumentos.

Los natives resuelven la interfaz SDK completa 014. El hook de autenticación usa
la ruta/factory del motor, sin convertir el getter de natives mediante cast.
Aunque ambas versiones se llamen 014, no se elige otra instancia por conveniencia.
Las firmas públicas SourcePawn se conservan. La versión 015 no se admite.

El fixture `tests/fixtures/gameserver014-prefix.json` conserva la declaración
independiente original. Las pruebas comprueban slots/prototipo y rechazan la
conversión implícita del prefijo a la interfaz completa. SourceHook real se prueba
con un objeto de motor 014, otro objeto SDK 014 y un doble 015 independiente:
solo el objetivo del motor debe emitir el forward. Un control negativo engancha
el objeto equivocado y debe fallar por pérdida del evento, no por compilación.

La prueba offline no emite un ticket auténtico. Una cuenta autenticada en SRCDS
requiere su propio ensayo sobre el hash del nuevo binario compilado con SDK 1.55;
la cobertura histórica no se transfiere automáticamente.
