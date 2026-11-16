# P1-B: AppID separado de la identidad Steam

Fecha: 2026-11-14. Rama: `develop`, base
`46e3ee05fc29ad090a1d6f7b1cb5e3dca9855350`, con P1-A local conservado.
Estado: implementación y regresión unitaria verificadas; aceptación en servidor
pendiente. Sin commit, release ni despliegue.

## Contrato antes y después

Se conservan los nombres, argumentos, orden y retornos públicos:

```sourcepawn
native bool SteamWorks_RequestStatsAuthID(int authid, int appid);
native bool SteamWorks_RequestStats(int client, int appid);
```

Antes, AuthID pasaba `params` a un helper que interpretaba la posición 2 como
universo. La ruta cliente podía hacerlo también en su fallback. Por ejemplo,
`RequestStatsAuthID(account, 550)` construía la identidad usando 550 como universo
en lugar de validar un AppID. `RequestUserStats` solo recibe una identidad y no
permite seleccionar un AppID arbitrario mediante un segundo argumento.

Ahora hay tres operaciones separadas dentro de
[ssnatives.cpp](../../extension/natives/ssnatives.cpp):

1. `GetServerStatsForApp(AppId_t)` exige Stats disponible, AppID no cero y Utils
   del gameserver disponible. El AppID debe coincidir con `GetAppID()` de esa
   interfaz; un AppID distinto, cero o contexto desconocido devuelve false sin
   ejecutar `RequestUserStats`. No se usa una constante 550 como sustituto de
   conocer el contexto.
2. `CreateStatsAccountIdentity(AccountID_t)` construye un usuario individual en
   universo público, sin acceso a `params`. Rechaza AccountID cero mediante
   `CSteamID::IsValid` y conserva todos los bits del identificador de 32 bits.
3. `GetStatsClientIdentity` exige cliente humano autorizado y AccountID validado
   de SourceMod. Usa `GetSteamId64(true)` y comprueba validez, tipo individual y
   coincidencia de AccountID. Solo si ese SteamID64 es cero permite fallback al
   AccountID validado en universo público. Una identidad completa inválida o
   inconsistente se rechaza; no se oculta con un fallback.

La ruta cliente sigue resolviendo una referencia/índice mediante SourceMod.
Cliente inexistente o desconectado conserva el error nativo anterior cuando el
AppID/contexto ya es válido. Bot, cliente sin autorización, AccountID cero o
identidad inválida devuelve false. El orden es Stats/AppID/Utils, cliente y
validación de identidad; no se consulta a Steam con una identidad rechazada.

Se retiraron los índices predeterminados del wrapper legacy local de identidad
para evitar reutilizarlo accidentalmente en solicitudes. Los lectores de stats
siguen pasando explícitamente sus posiciones 4/5, sin cambiar sus semánticas ni
las correcciones P1-A. Los helpers compartidos de `extension.cpp` y los natives
GameServer no se refactorizaron en esta entrega.

## Compatibilidad y migración

La compatibilidad de firma se mantiene. La compatibilidad de comportamiento
cambia deliberadamente: el segundo argumento es AppID, nunca universo. Los
callers legacy que pasaban `1` para universo público deben pasar el AppID real.
Valores extra no documentados no seleccionan universo/tipo en estas solicitudes.

El barrido P0 y su actualización focalizada encuentran solo el harness propio
como consumidor de `RequestStats*` fuera de declaraciones: usa `550` en ambas
variantes. No se encontraron usos deliberados del segundo argumento como
universo en ese alcance. Esto no prueba ausencia de plugins remotos/precompilados.

El argumento AuthID de una celda transporta **AccountID de 32 bits**, no SteamID64.
Los bits altos se preservan aunque la celda se vea negativa. Un SteamID64 ya
truncado por el llamador es indistinguible de un AccountID; esta API no puede
detectar ni recuperar esa pérdida. Se corrigieron los comentarios de ambas
solicitudes en el include, sin modificar declaraciones ni enums.

El retorno true conserva el significado de solicitud asíncrona aceptada
(`SteamAPICall_t` distinto de inválido). No certifica disponibilidad inmediata de
stats ni un resultado remoto. La adición de Utils como precondición puede hacer
que una petición antes aceptada devuelva false mientras no se pueda verificar
el AppID; esa política se aplica de forma explícita, sin suponer el contexto.

Los casos sintéticos con AppID 440 u otros bits prueban la comparación de
contexto; no amplían el soporte del fork a otro juego. El AppID efectivo y la
disponibilidad de Utils en el servidor L4D2 real todavía no se observaron en
esta ejecución. La expectativa de 550 requiere aceptación runtime.

## Pruebas deterministas

La suite [stats_requests_test.cpp](../../tests/stats_requests_test.cpp) ejecuta
los dos entrypoints de producción a través de su tabla real y utiliza el
`CSteamID` del SDK 1.58a identificado en P0. Los servicios SourceMod, Stats y
Utils son dobles. El doble de Stats captura la identidad completa entregada a
`RequestUserStats`, de modo que la corrección no se infiere del éxito de la red.
Las solicitudes corregidas no usan el doble del constructor legacy.

| Casos | Comprobación |
| --- | --- |
| AppID 550/contexto 550 | Identidad exacta, universo y tipo independientes del AppID |
| Otro contexto con AppID coincidente | Comparación de contexto sin hardcodear 550 |
| AppID ajeno, cero o valor legacy de universo | False y cero solicitudes Steam |
| Contexto desconocido, Utils o Stats ausentes | False y cero solicitudes |
| AccountID con bit alto, en ambas variantes | 32 bits preservados en la identidad de 64 bits |
| AppID con bit alto coincidente | Comparación como `AppId_t` de 32 bits, sin rechazo por signo de celda |
| AccountID cero | False, sin solicitud |
| Cliente inexistente/desconectado | Error nativo, sin solicitud |
| Bot o cliente sin autorización | False, sin lectura no validada ni solicitud |
| Identidad cliente válida con universo diferente | Universo validado preservado |
| SteamID64 ausente y AccountID validado | Fallback individual/público, nunca AppID como universo |
| SteamID64 inconsistente, tipo/universo/instancia inválidos | Rechazo sin fallback silencioso |
| Handle de llamada asíncrona inválido | Una solicitud realizada y retorno false |

Son 22 casos cliente y 12 AuthID: **34 casos nuevos**, junto a los **32 casos de
lectura de P1-A**. Se ejecutaron con UBSan en x86_64 (GCC 13.3.0) y x86
(`-m32`, GCC 12.2.0, contenedor P0). Las dos suites pasan sin warnings.

La suite nueva también se compiló contra una copia temporal del C++ anterior a
P1-B, conservando las correcciones P1-A. Ambas variantes originales fallan el
rechazo de AppID ajeno de forma determinista. Esa comprobación negativa no se
usa para afirmar que se reprodujo el comportamiento del antiguo constructor
compartido en una VM real.

## Validación y pendientes

| Prueba / nivel | Resultado y alcance |
| --- | --- |
| T01/U | Identidad exacta capturada; AppID separado del universo |
| T29/U | Cliente, fallback, bot, no autorizado, desconectado e inconsistencia |
| T30/U | Bits altos de AccountID/AppID, cero, AppID ajeno y tipos de identidad |
| Regresión P1-A/U | 32 casos de salida siguen pasando |
| T24/B | Firmas, enums, callbacks y registros públicos equivalentes a P0; cambios del include solo documentales |
| Harness SourcePawn | Compiló sin warnings con 1.12.0.7259; es el único consumidor localizado de las solicitudes afectadas |
| Unidad C++ con headers reales | Compila sin warnings como objeto ELF32 i386 |
| CI Linux | Paso actualizado para ambas suites usando el SDK de CI; lint validado localmente, ejecución remota pendiente |
| T01/I, T25/I, T29/I y T30/I | Pendientes de servidor Linux x86 identificado, plugins precompilados/recompilados y datos reales |

La compilación de la unidad modificada mantiene el procedimiento diagnóstico de
P1-A: se omite solo la opción de C `-Wno-pointer-to-int-cast` del comando GCC
registrado, conservando `-Werror` en el resto. No se cambió AMBuild. No se declara
un build completo aprobado: su bloqueo anterior sigue pendiente.

## Reproducción y evidencia

```bash
bash scripts/test-stats-natives.sh
```

El runner usa por defecto `sdk_158a/`; acepta `STEAMWORKS_SDK_DIR` para la copia
exacta del SDK. Las pruebas no enlazan `libsteam_api.so` ni inician un servidor.
Para x86 se usó el contenedor aislado de P0, sin red ni instalaciones nuevas.

El [registro de resultados](p1-b/validation-results.json) identifica la revisión
base, la huella del C++ local anterior a P1-B, las fuentes actuales, el SDK,
comandos, logs y resultados. Los logs/objetos locales permanecen en `.build/p1-b/`;
las instantáneas P0 y P1-A se conservan como evidencia histórica.

## Reversión y límites

Para revertir solo P1-B, retirar sus cambios de solicitudes/helper local, suite
de solicitudes, runner/CI y documentación, conservando P1-A y las correcciones
documentales anteriores. El fichero local previo se identificó por hash; no se
requiere restaurar todo el repositorio. No hubo despliegue que revertir.

La aceptación operativa de H08 sigue pendiente. No se resolvieron H09, selección
ABI de interfaces, detours, lifecycle ni compatibilidad HTTP de tres/cinco
argumentos. Una identidad correcta en un test local no garantiza que Steam
entregue stats para ese usuario.
