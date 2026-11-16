# T23: consumidores y resultados de forwards de P5-R13

2026-11-15. **T23 pasa en el alcance U/I documentado.** SRCDS Linux x86 en
`competitive_dev:27015`, iniciado con LinuxGSM, copia `addons/sourcemod_acceptance`,
flags/CFG habituales y sin contraseña añadida. Se instaló el payload del ZIP R13
verificado con el proceso detenido. ELF realmente mapeado:
`839ff0751b04141031bdc6dffc48662d0804f52227f2c91015bd87c6b84cf192`.
API declarada/registrada intacta; develop, sin commit ni producción.

## Qué se ejecutó

La extensión diagnóstica encuentra los **doce forwards gestionados existentes**
mediante `IForwardManager::FindForward`. No registra sustitutos ni modifica sus
receptores directamente. SourceMod administra altas/bajas al cargar/descargar
los SMX. El controlador no consume los forwards; tres copias del receptor tienen
identidades de fixture 1/2/3 y respetan las firmas públicas del include.

Los plugins ajenos se apartaron únicamente de la copia aislada, incluidos los de
autorrecarga/reinicio. Permanecieron las extensiones habituales, incluida
SourceTVSupport. Ningún ticket, token o identificador real se inspecciona/exporta:
los receptores retornan antes de leer parámetros de eventos ajenos al despacho
sintético activo. Los payloads de la prueba contienen NUL y bytes con bit alto.

| Forwards | Tipo real | Contrato comprobado |
|---|---|---|
| SW_OnValidateClient / SteamWorks_OnValidateClient | ET_Ignore | Dos AccountID sintéticos, orden y bits altos conservados |
| SteamWorks_SteamServersConnected | ET_Ignore | Cero parámetros, un callback por receptor |
| SteamWorks_SteamServersConnectFailure / SteamWorks_SteamServersDisconnected | ET_Ignore | EResult íntegro, sin provocar desconexión real |
| SteamWorks_OnClientGroupStatus | ET_Ignore | AccountID/grupo y booleanos Member/Officer en su posición |
| SteamWorks_RestartRequested | ET_Hook | Máximo no negativo; Handled continúa iteración, Stop la termina |
| SteamWorks_TokenRequested | ET_Ignore | Capacidad y copyback secuencial entre receptores y al llamador |
| SteamWorks_BeginAuthSession, interno ya existente | ET_Ignore | Bytes empaquetados, tamaño, padding y AccountID; mutación del primer receptor no llega al siguiente ni al origen; retorno Action ignorado |
| SteamWorks_GCSendMessage / SteamWorks_GCRetrieveMessage | ET_Event | Tipo con bit protobuf, bytes binarios, tamaños; máximo no negativo y todos los receptores, incluso resultado 4 |
| SteamWorks_GCMsgAvailable | ET_Ignore | Tamaño íntegro |

Ocho escenarios por forward: todos 0, todos 1, todos 3, todos 4, todos -1 y tres
combinaciones con resultados distintos, incluido Stop primero o en medio.
ET_Event parte de cero: todos -1 agrega **0**, sin inferir que el -1 llegue a la
rama especial del hook. El código de `Execute` se comprueba por separado del
resultado agregado. Con cero receptores, Execute preserva el sentinel de salida;
las rutas de producción que omiten ese despacho también tienen pruebas U.

## Resultado y controles

| Ronda | Orden de carga esperado | Comprobaciones acumuladas |
|---|---|---|
| Sin receptores | vacío | 524 |
| Uno | 1 | 1144 |
| Tres | 1,2,3 | 1951 |
| Descarga del segundo | 1,3 | 2665 |
| Recarga del segundo al final | 1,3,2 | 3472 |
| Descarga de todos | vacío | 3996 |
| Carga inversa | 3,2,1 | 4803 |

**672 despachos, 4803 comprobaciones positivas, cero fallos.** Mismo PID **121362**
y ticks de inicio **3206186** en las siete rondas. El orden es el observado en esta
carga de SourceMod, no una nueva garantía de prioridad entre plugins arbitrarios.
El orden entre eventos legacy/current de un productor no se infiere del bucle del
driver: sus pruebas de producción permanecen separadas de esta matriz de receptores.

Después se pidió deliberadamente el orden 1,2,3 manteniendo cargado 3,2,1. El
control produjo **104 fallos esperados** y `run_complete pass=0`; se conserva
completo, sin sumarlo como éxito ni confundirlo con un error de SourceMod.
HTTP final pasó con status 200, cuerpo/contexto correctos y handle cerrado.

Se repitieron las 14 suites x86 después del ajuste documental y del retorno
explícito del main del fixture reutilizado, incluidas CForward/VM, binding 014 y
propiedad de la VM instrumentada. Las **109 comprobaciones adicionales** del hook
de reinicio pasan bajo x86/UBSan y `-O3 -DNDEBUG`, sin desactivar checks. Tres
entrypoints SourcePawn compilan sin warnings. El ELF diagnóstico recompilado con
el script portable coincide byte a byte con el mapeado. La VM ASan es offline;
no se instaló en SRCDS ni se afirma instrumentación ASan del proceso del juego.

## Convención de reinicio y corrección documental

El include decía que Handled impedía el reinicio. La implementación histórica es
la contraria en la **consulta** `WasRestartRequested` cuando el SDK responde true:
Continue/Changed devuelven false; Handled/Stop conservan true. Sin receptores se
conserva la respuesta original; si el SDK responde false no se despacha el forward.
Esto se prueba con el cuerpo del hook de producción y dobles, no enviando un
reinicio real al gameserver. El desensamblado del baseline `9ca37aad…` compara con
3 y usa `setge`; el candidato compara con 2 y usa `setg`: mismo umbral firmado.

Se corrige **solo el comentario** de `steamworks.inc`, sin invertir comportamiento,
cambiar firma ni recompilar la extensión. La comparación de declaraciones,
registros, typesets y opcionalidad con P0 pasa. El main del fixture anterior recibe
`return 0` explícito para poder reutilizarlo como función de soporte sin warnings.

Los ZIP R13 quedan intactos y todavía contienen el comentario anterior. La
corrección y los nuevos tests se entregan en el archivo de seguimiento separado;
su incorporación al paquete corresponde al próximo refresco integrado de P5.

## Límites y restauración

Es despacho **sintético sobre forwards reales** de SourceMod en SRCDS. No acredita
que Steam emita cada callback, que GC exista en el perfil ni que el resultado se
observe en una operación real del SDK. El mapeo del hook de reinicio se acredita
con pruebas U y comparación binaria, no con un evento real. T11/T32 conservan
sus pendientes; tampoco se acepta la pila completa de consumidores de T25 ni
operación sostenida de T33.

Quit controlado sin marcadores de crash ni logs de error SourceMod. Selector,
cuatro hashes protegidos y módulo original restaurados; DEV conectado a Steam,
PID nuevo **123822**; laboratorio y fixture HTTP retirados. No se aplicó un ZIP
rollback en sitio: T35 mantiene el alcance del ensayo R9.

[Resultados y hashes](forward-consumers-srcds/results.json),
[control negativo](forward-consumers-srcds/negative-order.json),
[pruebas de reinicio](forward-consumers-srcds/restart-semantics-final.log),
[API](forward-consumers-srcds/public-api-check.json),
[restauración](forward-consumers-srcds/cleanup.json),
[matriz vigente](p5/validation-results.json).
