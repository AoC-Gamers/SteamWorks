# T11: interfaces y capacidades de P5-R14 en SRCDS x86

2026-11-15. **T11 pasa para el perfil observado**, con nueve familias clasificadas:
ocho interfaces disponibles con ejercicio mínimo y Matchmaking no proporcionada
en este contexto gameserver. No equivale a aceptar todos los métodos del SDK,
GC remoto ni todos los consumidores. Quedan T25/T29/T32/T33 abiertos.

Se usó `competitive_dev`, LinuxGSM `/app/l4d2server`, copia
`addons/sourcemod_acceptance`, puerto 27015 y flags/CFG habituales, sin añadir
contraseña. Payload instalado desde el ZIP R14 verificado con SRCDS detenido.
ELF mapeado `839ff0751b04141031bdc6dffc48662d0804f52227f2c91015bd87c6b84cf192`;
include y API pública intactos. No se modifica código de runtime, hace commit,
publica ni despliega en producción.

## Método y matriz

El observador invoca **los getters del candidato real** mediante SDKCall_Raw;
no recompila un resolver alternativo ni cambia los punteros de sus interfaces.
El host comprueba SHA/inode del ELF mapeado, rangos ejecutables de los nueve
getters y rango escribible del slot del resolver. El slot se deriva del símbolo
g_SteamWorks y de las instrucciones PIC de sm_IsConnected en ese ELF exacto.
Luego un helper ELF32 llama métodos virtuales con los tipos SDK declarados y el
prefijo 014 independiente. No se hacen llamadas sobre punteros null.

Las versiones siguientes son las **permitidas por el registro** y las rutas son
las del getter de producción. No se afirma haber trazado cada rama interna de
caché/factory ni que haya negociación automática de versiones.

| Familia / versión admitida | Getter / ruta | Estado y ejercicio mínimo |
|---|---|---|
| Client / SteamClient020 | GetSteamClient; caché SDK o factory de respaldo | Disponible; GetIPCCallCount. La consulta puede reiniciar ese contador diagnóstico |
| GameServer / SteamGameServer015 | GetGameServer / GetISteamGameServer(user,pipe) | Disponible; BLoggedOn, natives públicos de carga/conexión/IPv4 |
| Utils / SteamUtils010 | GetUtils / GetISteamUtils(pipe) | Disponible; AppID 550 y universo público, separados |
| Networking / SteamNetworking006 | GetNetworking / GetISteamNetworking(user,pipe) | Disponible; IsP2PPacketAvailable, sin consumir ni enviar paquetes |
| Stats / SteamGameServerStats001 | GetServerStats / GetISteamGameServerStats(user,pipe) | Disponible; lectura local rechazada con identidad cero; natives preservan sentinels enteros/float al fallar |
| HTTP / STEAMHTTP_INTERFACE_VERSION003 | GetHTTP / GetISteamHTTP(user,pipe) | Disponible; crear/liberar petición y smoke público real contra fixture loopback |
| Matchmaking / SteamMatchMaking009 | GetMatchmaking / GetISteamMatchmaking(user,pipe) | Devuelve null de forma repetida; no se llama GetFavoriteGameCount ni se prueban versiones alternativas. No tiene consumidores/natives en el runtime actual |
| GC / SteamGameCoordinator001 | GetGameCoordinator / GetISteamGenericInterface(user,pipe) | Disponible; cola vacía, RetrieveMessage devuelve NoMessage; envío público interceptado localmente con parámetros/resultados comprobados |
| Autenticación / SteamGameServer014 | GetGameServerAuth014 / SteamInternal_FindOrCreateGameServerInterface(user) | Disponible; BLoggedOn mediante el tipo 014, separado de 015. No se fabrican tickets |

Todos los getters se invocan dos veces por ronda y devuelven un resultado estable.
La ausencia de Matchmaking es una observación de este contexto, no una afirmación
universal sobre esa interfaz ni permiso para adivinar otra ABI.

## GC presente, rechazado y restaurado

1. **Defaults, PID 128115:** ocho interfaces disponibles, Matchmaking null. GC
   IsMessageAvailable devuelve false y RetrieveMessage devuelve NoMessage sin
   consumir datos. El native de envío alcanza el hook y su forward de prueba;
   este devuelve InvalidMessage y supercede al proveedor antes de enviar a Steam.
2. **Versión desconocida válida, PID 132249:** se añade solo al laboratorio un
   gamedata multilinea con SteamGameCoordinatorVersion=SteamGameCoordinator999.
   El registro rechaza la versión, devuelve GC null y el native devuelve
   NotLoggedOn. El error de rechazo se registra **una sola vez** pese a consultas
   repetidas y espera; las demás interfaces mantienen sus estados y HTTP pasa.
3. **Defaults restaurados, PID 134234:** se retira exactamente ese archivo propio
   y se arranca de nuevo con LinuxGSM. GC vuelve a estar disponible, repite
   cola vacía/NoMessage e interceptación local; HTTP y demás interfaces pasan.

Cinco rondas aceptadas: **144 comprobaciones, cero fallos aceptados**. Los checks
acumulados por etapa son 33, 55 y 56. No se afirma continuidad de PID entre
arranques fríos ni recuperación sin destruir el runtime de T32.

El envío de fixture no alcanza el backend GC. No se acredita sesión GC remota,
protocolo de aplicación ni entrega/recepción real de mensajes. Si hubiera datos
reales en cola, el helper omitiría RetrieveMessage para no consumirlos; en estas
rondas aceptadas la cola estuvo vacía. No se exportan IP, identidades, tickets,
stats personales ni configuraciones privadas.

## Intentos inválidos preservados

El primer observador suponía erróneamente que Matchmaking debía existir: guardó
un fallo de expectativa, aunque el getter devolvió null correctamente. El
observador corregido clasifica la ausencia y no llama métodos sobre ella.

El primer control GC usó un gamedata de una línea rechazado por SMC: error 6,
línea 1, columna 88, `Section declared with unknown tokens`. El PID 130184 mantuvo
los defaults; ese intento **no** demuestra rechazo de versión. Se conservan sus
trazas fallidas y metadatos del log. El control se repitió con formato válido.

Los cuatro renglones de ese error de parser se contabilizan por separado mediante
un corte de líneas/SHA previo; el colector rechaza cualquier error nuevo ajeno al
único rechazo GC esperado. Los logs no se borran ni las comprobaciones se suprimen.
El tamaño de los logs de error al quit final coincide con la última clasificación:
no se añaden errores durante ese cierre.

## Validación, restauración y entrega

Se repiten 15 suites x86, incluyendo API contra P0, registro/resolver, binding 014,
CForward/VM, contexto streaming y propiedad de VM instrumentada. Los dos entrypoints
SourcePawn y el helper ELF32 compilan sin warnings. La VM ASan sigue siendo offline;
no se instala en SRCDS. Los archivos de producto y API anteriores no cambian.

Los arranques probados terminan con quit controlado sin marcadores de crash.
Se restaura el selector original y los cuatro hashes protegidos; DEV queda
conectado a Steam, PID 136214, y se retiran laboratorio y fixture HTTP. No se aplica
un ZIP rollback en sitio ni se acepta el cierre limpio del baseline conocido.

Los ZIP R14 y los informes previos permanecen intactos. Las fuentes nuevas y la
evidencia se entregan en un archivo separado, para el siguiente refresco de P5.

[Resultados y hashes](interface-capabilities-srcds/results.json),
[matriz estructurada](interface-capabilities-srcds/interface-matrix.json),
[rechazo válido](interface-capabilities-srcds/rejected-valid-repeat.json),
[restauración de capacidades](interface-capabilities-srcds/restored-repeat.json),
[restauración DEV](interface-capabilities-srcds/cleanup.json),
[matriz global](p5/validation-results.json).
