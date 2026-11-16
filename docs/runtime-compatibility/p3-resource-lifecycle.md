# P3: ciclo de vida y propiedad de recursos

Fecha: 2026-11-14. Rama `develop`, base
`46e3ee05fc29ad090a1d6f7b1cb5e3dca9855350`. Se conservan los cambios locales
anteriores de documentación y P0/P1/P2. Sin staging, commit, publicación ni
despliegue. API pública SourcePawn conservada; aceptación en SRCDS pendiente.

## Problemas y comportamiento resultante

Antes, los destructores de hooks obtenían de nuevo la interfaz del resolver.
Si este ya apuntaba a una instancia distinta, la retirada podía dirigirse a
esa otra instancia y dejar el registro anterior. El shutdown observado solo
retiraba GameServer, mientras GC y los callbacks HTTP conservaban recursos.
Además, el cierre de HTTP encolaba una función de la extensión para otro frame;
el objeto y esa función podían seguir pendientes al descargar su componente.
Los controles negativos reproducen estos comportamientos con servicios
simulados; no se afirma un crash observado en un servidor real.

Ahora cada grupo guarda la instancia enganchada y los tres IDs devueltos por
SourceHook. Se retiran esos IDs sin consultar getters. Instalar sobre la misma
instancia es idempotente; cambiar de instancia retira primero los registros
anteriores. Si cualquiera de las tres instalaciones falla, se revierten los
IDs obtenidos y se desactiva la espera de ese grupo hasta un nuevo inicio
explícito. Cada suscripción por frame se registra y retira una sola vez.

La disponibilidad general de la extensión se bloquea antes del desmontaje.
Todos los getters del resolver respetan ese bloqueo, incluso con punteros
cacheados o con el global SteamClient todavía presente. Un init original que
falla no llama al coordinador de inicio; uno exitoso permite reconciliar ambos
grupos. El detour conserva la llamada y el resultado del original.

Hay un cambio deliberado de carga: **si no se puede preparar el par de detours
de inicio/shutdown, la extensión devuelve error de carga antes de registrar
los consumidores dependientes**. Sin ese observador no puede garantizar la
retirada de interfaces prestadas antes del shutdown. También se revierte una
carga que no logra crear el tipo de handle HTTP. Los fallos parciales conservan
un diagnóstico y permiten repetir el cleanup sin doble liberación. Esto no
valida el prototipo binario de los detours: esa aceptación sigue siendo P4.

## Propietarios

| Recurso | Propietario | Fin de vida |
| --- | --- | --- |
| Interfaces Steam | Runtime; la extensión solo las toma prestadas | Reset de cachés después de desmontar consumidores; nunca `delete` |
| Par de detours | `SteamWorksGSDetours` | Destrucción y limpieza de trampolines al descargar; rollback si falla creación |
| Hooks GameServer/GC | Su grupo, con instancia e IDs propios | `Stop`, cambio de instancia o destructor; rollback por instalación parcial |
| Espera por frame | El mismo grupo de hooks | Al instalar, detener, fallar instalación o destruir el grupo |
| Seis suscripciones Steam generales | `SteamWorksForwards` | `Unregister` antes de soltar los forwards; registro de nuevo al iniciar |
| Tipo de handle HTTP y listener de plugins | `SteamWorksHTTP` | Desregistrar listener, retirar tipo y drenar peticiones durante destrucción |
| Petición Steam HTTP | `SteamWorksHTTPRequest`, sobre el `ISteamHTTP*` que la creó | Release único antes del shutdown o al cerrar; nunca adquirir otra interfaz en el destructor |
| Handle público SourceMod | SourceMod y el plugin propietario | `OnHandleDestroy` cierra el objeto; clones siguen la propiedad del sistema de handles |
| Call result de finalización y callbacks de streaming | La petición correspondiente | Cancelar/desregistrar al invalidar, cerrar o descargar su propietario |
| Forwards HTTP | La petición correspondiente | Soltar al invalidar, una vez que termine la invocación activa más externa |

Los dueños de hooks, peticiones, gestor HTTP y callbacks generales no se pueden
copiar. Los handles públicos SourceMod y los handles Steam siguen separados;
no cambia su representación pública.

## Secuencia de parada y reinicio

`SteamWorks::RuntimeStopped`, invocado **antes** del shutdown original:

1. Bloquear adquisiciones y trabajo nuevo.
2. Retirar las esperas por frame y los hooks GameServer y GC por sus IDs.
3. Desregistrar las seis suscripciones de callbacks generales.
4. Invalidar las peticiones HTTP: cancelar el call result, desregistrar streaming,
   liberar su handle Steam mediante la interfaz capturada y retirar forwards
   cuando no haya una invocación activa.
5. Resetear las cachés de interfaces.
6. El wrapper permite ejecutar el shutdown original exactamente una vez.

Repetir la parada no duplica retiradas. El inicio original exitoso habilita de
nuevo HTTP, callbacks generales y reconciliación de ambos grupos de hooks.
Los handles HTTP de una generación detenida permanecen inválidos para operar,
aunque SourceMod aún no los haya cerrado: no se reactivan al reiniciar. Con un
runtime nuevo disponible, usarlos produce error nativo; durante la ausencia de
HTTP se conserva el retorno 0 de capacidad ausente. Se deben crear peticiones
nuevas para la nueva generación.

La descarga de extensión marca primero que no se podrá reiniciar, ejecuta la
misma parada y destruye componentes. El gestor HTTP retira el tipo de handle y
drena su registro sin dejar acciones de frame pendientes. Interfaces y gamedata
se destruyen al final. La carga tardía puede enganchar el runtime ya disponible
o esperar por frame; no se ha añadido ningún reset por cambio de mapa ni por
mera pérdida de conexión con Steam.

Los contadores de generación impiden continuar un evento anterior si un forward
provoca una parada y un arranque durante la misma ejecución, incluso si la
instancia reemplazante ocupa la misma dirección. Se conservan los resultados y
el orden de los forwards normales, y la extracción GC existente sigue invocando
su original una vez. Esto no rediseña los contratos de buffers GC de P4/P5.

## Cierre HTTP durante callbacks

Cada invocación asíncrona y el helper síncrono de lectura del body mantienen un
`DispatchScope`. Cerrar el handle invalida la petición y cancela inmediatamente
suscripciones y propiedad Steam. Si se está ejecutando un callback, la petición
y sus forwards se mantienen vivos hasta que vuelva la invocación más externa.
Entonces se libera el objeto, sin programar una función para otro frame. Este
mecanismo también cubre callbacks anidados.

El listener `OnPluginWillUnload` invalida las peticiones del plugin propietario
antes de `OnPluginEnd` y retira sus funciones de los forwards de peticiones
ajenas. El cleanup automático posterior de handles sigue cerrando los objetos;
no se intenta liberar un handle público prestado manualmente. Un plugin que se
está descargando ya no puede seguir operando sus peticiones desde `OnPluginEnd`.

La finalización utiliza el `CCallResult` de su llamada asíncrona de 64 bits.
Cabeceras y datos de streaming utilizan `CCallbackManual` de gameserver, que es
el mecanismo declarado para esos eventos en el SDK local; antes estaban ligados
como call results. Cada receptor filtra el handle de petición además de comprobar
que su generación siga activa. Se conserva el contexto y los 6/4/6 argumentos de
los callbacks públicos. Un error al reservar cualquiera de los forwards nuevos
revierte la configuración pendiente antes de cambiar bindings existentes.

La generación es una comprobación adicional: la protección del receptor depende
también de cancelar/desregistrar los callbacks y mantener el objeto durante la
invocación. El diseño sigue el despacho serial de SourceMod/Steam en el hilo del
servidor; no añade sincronización para llamadas concurrentes desde otros hilos.

## Validación determinista

```bash
bash scripts/test-resource-lifecycle.sh
bash scripts/test-typed-boundary.sh
bash scripts/test-stats-natives.sh
bash scripts/test-detours.sh
bash scripts/test-public-ip.sh
AMTL_DIR=<sourcemod>/public/amtl bash scripts/test-steamid-string.sh
# Validación adicional local en x86_64:
TEST_CXXFLAGS=-fsanitize=address bash scripts/test-resource-lifecycle.sh
```

La suite de lifecycle aporta **207 comprobaciones por arquitectura**:

- 98 HTTP: cierre normal, cierre dentro de cada callback y helper síncrono,
  anidamiento, generaciones, descarga de propietario o de otro receptor,
  cancelación real de las clases SDK con transporte simulado, fallo de handle,
  rollback de forwards, fallo de tipo y 50 ciclos sin registros retenidos.
- 20 callbacks generales: registro idempotente, argumentos y compatibilidad de
  forwards legacy/actual, consumidores vacíos, retirada, reinicio y reentrada.
- 42 hooks: espera idempotente, instancia de instalación frente a reemplazo,
  fallo en cada posición de ambos grupos, rollback y reentrada por generación.
- 47 coordinación: código real de carga, parada, reinicio y descarga con
  componentes simulados; orden, bloqueo previo, observador imprescindible,
  carga parcial y cleanup repetido.

Se ejecutaron en Linux x86_64/GCC 13 y x86/GCC 12 con UBSan sin recuperación.
La misma suite pasó además en x86_64 con ASan/UBSan, con detección de fugas
activada explícitamente (`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1`). Se usan clases reales `CCallResult`/`CCallback` del SDK en
las pruebas de suscripciones; el registro, transporte de eventos, SourceMod y
SourceHook se simulan. El nuevo paso de prueba se configura en CI Linux x86;
no se ha ejecutado CI remoto.

También pasaron todas las regresiones P1/P2: 171 escenarios previos por
arquitectura; el resolver ahora tiene 192 comprobaciones, incluidas 8 nuevas
sobre interfaces cacheadas/globales durante parada; identidad tiene 46 en
x86_64 y 45 en x86; HTTP/GC conserva sus 57; las dos compilaciones negativas
siguen rechazando mezcla de IDs y cambio de versión del SDK.

Dos probes contra la copia anterior a P3 fallan como se esperaba: uno detecta
el getter y los hooks antiguos retenidos al destruir frente a una instancia
reemplazante, y otro detecta el borrado HTTP encolado para otro frame. Son
regresiones sintéticas de comportamiento, sin afirmación de crash real.

Se comparó la API con P0 ignorando líneas: mismas declaraciones, enums,
callbacks, natives/forwards registrados, binding y lista opcional. El escaneo
conserva 2670 archivos SourcePawn y las discrepancias de consumidores ya
inventariadas. P3 no modifica `.sp` ni `.inc`. Se compilaron ocho objetos ELF32
con SDK, SourceMod y SourceHook reales: resolver, callbacks generales, ambos
grupos de hooks, detours, gestor HTTP, petición HTTP y extensión. Los comandos
diagnósticos conservan `-Werror` y omiten únicamente el flag C preexistente
`-Wno-pointer-to-int-cast`; no equivalen a un build enlazado nuevo.

Resultados, hashes y propietarios: [validation-results.json](p3/validation-results.json).
Logs, comandos, copias previas y probes: `.build/p3/` local. Los resultados de
P0/P1/P2 se conservan como capturas históricas.

## Aceptación pendiente

T08/T09 y T20/T21/T23 tienen evidencia unitaria para los casos descritos. El
modelo de SourceHook no demuestra modificación o retirada de una vtable real,
y los forwards simulados no ejecutan una VM SourceMod real. Las pruebas locales
no certifican el transporte ni la cola interna de un runtime Steam identificado.

T16-I/R, T08-R y la aceptación integrada de T20/T21 siguen pendientes: carga
tardía en SRCDS, cambios de mapa, shutdown/reinicio real, descarga de plugins
reales y ejecución prolongada del binario. No se valida descargar el propio
módulo binario de extensión mientras uno de sus frames C++ siga en la pila.
P4 sigue pendiente para los perfiles, prototipos y rutas internas; no se añaden
patrones ni perfiles por analogía. Tampoco hay build completo enlazado nuevo,
validación Windows, despliegue ni publicación.
