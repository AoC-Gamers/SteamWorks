# Harnesses de aceptación en SRCDS

Son plugins de laboratorio; no forman parte del paquete de la extensión. Usar
un `sm_basepath` aislado y el supervisor del servidor (LinuxGSM en
`competitive_dev`), conservando una copia privada de la configuración antes de
cambiar el selector. No reemplazar SourcePawn ni bibliotecas Steam.

Compilar los harnesses `.sp` con `spcomp64`, el include candidato en `scripting/include`
y los includes del SourceMod del laboratorio. Cada compilación debe terminar
sin warnings. El fixture Python usa únicamente loopback:

```sh
python3 tests/runtime/http_fixture.py --cert /ruta/cert.pem \
  --key /ruta/key.pem --output /ruta/fixture-counts.json
```

El certificado de laboratorio debe ser autofirmado. La prueba TLS exige
verificación y espera su rechazo. No reutilizar claves de otro servicio. El
puerto 27883 debe permanecer cerrado para el caso de conexión rechazada.

Desde la consola del servidor, ejecutar por separado:

```text
sm_sw_accept status
sm_sw_accept http
sm_sw_accept stats
sm_sw_accept clients
sm_sw_accept cancel
sm_sw_accept repeat
```

Esperar al menos seis segundos después de HTTP/cancelación. `http` valida texto,
binario con ceros, cuerpo vacío, HTTP 503, timeout, TLS inválido, conexión
rechazada y streaming. Cada callback comprueba las dos celdas del contexto;
`headers` y `data` imprimen `context=1` únicamente si coinciden. `completed`
cuenta callbacks recibidos, no pruebas aprobadas. Los fallos permanecen en
`failed`; una recarga reinicia los contadores. No atribuir todo el historial al
último contador.

`clients` requiere un humano autenticado y dentro del juego: compara la identidad con SourceMod,
comprueba conteos y solicita stats. La lectura diferida contrasta el stat entero
por índice de cliente con el mismo stat por AccountID; antes de usar un índice
vuelve a verificar que siga en juego y corresponda a esa cuenta. `authstats <AccountID autorizado>` permite
una lectura independiente; mantener privada la consola con ese comando.
`metadata` comprueba solamente la invocación de setters y modifica metadatos del
servidor; no constituye validación semántica ni debe preceder una prueba de acceso.

Para probar el dueño de una petición pendiente, cargar
`disabled/steamworks_pending_owner` y descargar exactamente ese mismo nombre
antes de tres segundos. Después de `ended` no debe aparecer `callback`. Una
descarga fallida por nombre incorrecto invalida esa ejecución. Los forwards públicos de validación se cuentan por separado del hook interno
`BeginAuthSession`, y `status` imprime ambos contadores. Una recarga del harness
los reinicia y los eventos anteriores no se atribuyen a receptores nuevos.
Los forwards de autenticación solo se cuentan; nunca se inspeccionan ni registran tickets.

Conservar evidencia filtrada y hashes de módulos realmente mapeados. Registrar
por separado los resultados de lanzamiento directo y los de LinuxGSM. Al
terminar, detener el laboratorio, restaurar selector y servicio originales y
retirar fixture, certificados y archivos propios. Véase el
[informe de P5-R2](../../docs/runtime-compatibility/srcds-acceptance.md).

## Stats float y claves inexistentes

`steamworks_stats_acceptance.sp` consulta exclusivamente stats del único humano
autorizado presente. No escribe stats ni registra identidades o valores personales.
Generar primero el include de claves públicas del schema AppID 550:

```sh
python3 scripts/generate-stats-acceptance-schema.py .build/mi-stats/stats_schema_keys.inc
spcomp64 tests/runtime/steamworks_stats_acceptance.sp -i.build/mi-stats \
  -iscripting/include -i/ruta/sourcemod/scripting/include \
  -o.build/mi-stats/steamworks_stats_acceptance.smx
```

Cargar solo en el árbol aislado. Desde la consola, ejecutar `sm_sw_stats_accept`
con un humano dentro del juego. Solicita stats por cliente/AccountID, espera una
lectura entera conocida y comprueba una clave ausente del schema con las cuatro
rutas de lectura. Las salidas usan sentinels distintos, incluidos bits NaN, que
deben permanecer idénticos cuando falla el getter.

Luego recorre las claves en lotes de 16 por timer: un resultado float positivo
requiere éxito por ambas rutas, valores finitos y bits coincidentes. No infiere
el tipo de una clave por su nombre o su valor default. Los rechazos float deben
conservar los sentinels. Repite la clave inexistente al terminar y exige al menos
un float leído con éxito y cero fallos para aprobar. Solo imprime claves públicas
y booleanos de comprobación. Revalida userid/AccountID en cada timer y cancela al
salir el cliente, cambiar el mapa o descargarse el plugin.

## Reversión desde ZIP

`steamworks_rollback_acceptance.sp` usa solo natives comunes al include candidato
y al baseline. Compilar contra ambos sin warnings. `sm_sw_restore_check http`
comprueba carga/conexión, HTTP 200, cuerpo fijo y contexto sintético; cierra el
handle mediante un timer después del forward, para no exigir al baseline antiguo
la mejora de cierre seguro dentro del callback del candidato.

Usar exclusivamente el árbol aislado. Probar candidato, detener completamente
SRCDS, aplicar el ZIP de reversión en ese mismo árbol sin cambiar su selector,
comprobar hashes/ausencias/configs ajenas y arrancar de nuevo con LinuxGSM.
Verificar el módulo realmente mapeado y repetir el smoke. Registrar por separado
los marcadores de crash al salir: un mensaje de shutdown no certifica salida
limpia. El ensayo P5-R5 y la incidencia comparativa están en
[el informe de reversión](../../docs/runtime-compatibility/p5-r5-rollback-srcds-acceptance.md).

## Matriz de consumidores de forwards

`steamworks_forward_fixture.cpp` es una extensión **solo de laboratorio**. Busca
los forwards gestionados ya registrados por el candidato mediante `FindForward`;
no crea sustitutos, modifica hooks ni llama al SDK. Compilar ELF32 con
`scripts/build-forward-fixture.sh`, fijando `SOURCEMOD_DIR` y `TEST_BUILD_DIR`.
El helper completa los operadores sized-delete de la copia local del SDK base,
sin modificar los archivos del producto ni desactivar warnings.

Compilar `steamworks_forward_receiver.sp` y cargar copias del mismo SMX llamadas
`steamworks_forward_receiver_1.smx`, `_2.smx` y `_3.smx`. El controlador
`steamworks_forward_acceptance.sp` no consume ninguno de los forwards probados.
Desde la consola del servidor, `sm_sw_forward_check 0`, `1`, `123`, `13`, `132`
y `321` indican el orden esperado de los receptores cargados. Cambiar la carga
entre rondas: el número no crea ni reordena receptores. Un orden falso debe fallar.

Ejecutar únicamente con plugins ajenos retirados de la copia aislada. Los datos
son sintéticos; los receptores ignoran eventos reales antes de leer sus argumentos.
Las mutaciones de token y ticket solo ocurren durante el despacho sintético activo.
El helper prueba los doce forwards existentes, tipos ET_Ignore/ET_Hook/ET_Event,
orden/conteo, Stop, agregación, parámetros binarios, ausencia de copyback del ticket
y copyback secuencial del token. No prueba disponibilidad de GC ni genera eventos
reales de autenticación, pérdida de conexión, LogOn o reinicio de Steam.

`scripts/test-restart-forward-semantics.sh` contrasta el mapeo del hook de reinicio
con el cuerpo de producción y dobles del SDK/SourceHook/forward, bajo x86/UBSan y
`-O3 -DNDEBUG`. La convención histórica conserva la consulta verdadera del SDK para
Handled/Stop y la suprime para Continue/Changed cuando hay receptores; sin ellos
conserva el resultado original. No invertir esa convención durante una aceptación.
La prueba incluye el fixture previo; su main ahora retorna explícitamente cero.

Véase el [informe de forwards](../../docs/runtime-compatibility/forward-consumers-srcds-acceptance.md).

## Matriz de interfaces y capacidades

`steamworks_capability_acceptance.sp` invoca los nueve getters **del candidato
real**, por SDKCall_Raw, y entrega sus punteros a las consultas tipadas del helper
`steamworks_capability_fixture.cpp`. Compilar ELF32 con
`scripts/build-capability-fixture.sh`, usando `SOURCEMOD_DIR`, `STEAMWORKS_SDK_DIR`
y un `TEST_BUILD_DIR` nuevo. El helper no crea un resolver alternativo, no retiene
propiedad de interfaces y nunca hace shutdown de Steam.

El host debe verificar antes el SHA del ELF mapeado, inode, rangos ejecutables de
los getters y el slot escribible del puntero al resolver. En el ELF aceptado
839ff075, ese slot se deriva del desensamblado de sm_IsConnected y del símbolo
g_SteamWorks; las direcciones de proceso no se adivinan ni se publican en trazas.
`sm_sw_capabilities <slot> <nueve getters>` solo acepta consola del laboratorio;
`sm_sw_capabilities check` repite la matriz y `rejected` exige GC deshabilitado.
Descargar el harness libera los nueve handles de SDKCall.

Las consultas mínimas incluyen AppID/universo, conexión de 015/014, peek P2P,
stats locales con identidad cero, creación/liberación HTTP y cola GC vacía.
El contador IPC es diagnóstico y su lectura puede reiniciarlo según el SDK.
Matchmaking puede devolver null para el contexto gameserver: registrar ausencia,
sin llamar métodos sobre ese puntero ni probar versiones alternativas.

GC presente se prueba con IsMessageAvailable y RetrieveMessage solo si la cola
está vacía, sin consumir mensajes reales. El envío público de fixture se intercepta
con su forward y devuelve InvalidMessage, supercediendo al proveedor; no se envían
mensajes arbitrarios al backend. GC ausente devuelve NotLoggedOn.

Para rechazar una versión desconocida, usar un gamedata multilinea propio solo
en la copia aislada con Keys/SteamGameCoordinatorVersion=SteamGameCoordinator999.
Exigir arranque nuevo, error de versión una sola vez, GC null y otras interfaces/
HTTP operativas; retirar exactamente ese archivo y repetir con otro arranque.
Un error de parseo del archivo invalida el control: no demuestra rechazo de versión.
No modificar configuraciones ni bibliotecas de la instalación normal.

Véase el [informe de interfaces](../../docs/runtime-compatibility/interface-capabilities-srcds-acceptance.md).

## Consumidores precompilados/recompilados y contratos

`steamworks_consumer_observer.sp` carga con los consumidores aparte y comprueba
su estado Running, callback IP de AoC y API/fallback local de Family Share. Requiere
el include externo `l4d2_familyshare_core.inc` del proveedor; no copiarlo al ZIP.
La presencia de su biblioteca se mantiene en un único PluginRuntimeState y sus
natives se llaman solo fuera de los callbacks de alta/baja.

Antes de cargar AoC/Family Share, el observador bloquea exclusivamente los `exec`
de sus dos CFG originales en la copia de laboratorio. Los archivos deben existir
previamente para impedir que AutoExecConfig los cree. Se verifican SHA y mtime;
se usan defaults/settings de laboratorio, sin claves API, forcematch ni guard de
reinicio. El callback IP de AoC se invoca explícitamente; hostip/net_public_adr se
comparan en memoria y restauran de inmediato, sin exportarlos. Descargar los
consumidores antes de pedir la entrada humana evita sus flujos de bienvenida.

Conservar por separado SMX **preexistentes** con procedencia/SHA y recompilaciones
frescas. La carga/importación no equivale a probar toda la lógica de negocio.
Para Family Share sin clave solo se acredita fallback; no perfil HTTP positivo,
licencia prestada real ni toda su integración con gestores.

Compilar `steamworks_consumer_contract.sp` dos veces, contra el include original
archivado y el actual, como `contract_legacy.smx` y `contract_current.smx`. Son
fixtures nuevos compilados contra dos contratos, no binarios históricos. Sus
comandos `sm_sw_contract_legacy` y `sm_sw_contract_current` usan el humano autorizado
presente, sin registrar identidad ni stats. H08 comprueba AppID 550 por ambas rutas,
rechazo de 0/1 y lectura coincidente de un stat conocido tras espera acotada; H09
comprueba conteo/NUL y capacidades suficiente, truncada, uno y cero. Las lecturas
fallidas conservan sentinels y el userid se revalida en los timers.

`steamworks_legacy_http_consumer.sp` reproduce una declaración externa de tres
argumentos. Usa un handle válido sin enviar la petición y captura el error 23 del
lector actual que requiere cinco argumentos, con buffer intacto. La excepción se
registra normalmente; solo se admiten ese mensaje, native y plugin exactos.
Ese reproducer no acredita ejecutar la aplicación Connect completa ni corrige su
include/llamadores. El mismo call shape debe fallar al compilar con el include
actual. No ampliar la API del productor para ocultar consumidores incompatibles.

Véase el [informe de consumidores](../../docs/runtime-compatibility/consumer-compat-srcds-acceptance.md).

## Pérdida y recuperación de conexión Steam

`steamworks_connection_observer.sp` observa forwards reales del SDK y cambios de
`IsConnected`, comprobando que `IsLoaded` permanece true. Compilar directamente
con spcomp, los includes actuales SteamWorks/SourceMod y un destino nuevo. No
requiere el include externo Family Share. Cargar antes de la conexión inicial.

El host verifica SHA/inode del ELF mapeado, rangos ejecutables de los nueve getters
y el slot del resolver con el mismo perfil auditado de T11. El comando exclusivo
de consola `sm_sw_connection_bind <slot> <nueve getters>` guarda sus resultados
sin publicar punteros. `sm_sw_connection_http retain` conserva una respuesta local;
`sm_sw_connection_check baseline|down|up` exige conexión acorde, interfaces iguales,
respuesta retenida legible y stats negativos sin cambios de salida. El modo up
exige dos forwards de conexión y al menos uno de desconexión. Ejecutar
`sm_sw_connection_http fresh` también durante el corte y tras recuperar; comprobar
completion, contexto, cuerpo, cierre y ausencia de errores. Al descargar se
liberan timer, peticiones y SDKCall handles. No simular los forwards de Steam.

En `competitive_dev` la red es host: un corte general afectaría servicios ajenos.
El ensayo usa reglas temporales IPv4/IPv6 con cgroup exacto del contenedor y UID
de LinuxGSM, únicamente mientras está activo el laboratorio con plugins mínimos.
Se conservan loopback, DNS y tráfico saliente de juego 27015. Todo otro egreso
coincidente se descarta; no se afirma que el filtro conozca destinos Steam.
Verificar cgroup/inode, un único SRCDS y contador DROP. Un guard independiente
retira sus saltos/cadenas al recibir stop, ante excepción o por límite temporal.
El watchdog es protección disponible: el ensayo acredita la retirada explícita.
Nunca vaciar OUTPUT ni tablas/cadenas ajenas. Exigir una verificación posterior
de ausencia de los saltos/cadenas propios, y restaurar DEV mediante LinuxGSM.

Un ciclo acotado no acepta desconexiones indefinidas, T33, GC remoto ni stats
positivos humanos. Véase el [informe de conexión](../../docs/runtime-compatibility/connection-srcds-acceptance.md).

## Operación sostenida y recursos

`steamworks_soak_observer.sp` usa ocho slots con secuencias/contextos únicos y
callbacks de cierre explícito. Compilar con spcomp Linux y los includes actuales
SteamWorks/SourceMod; no requiere includes externos. El comando exclusivo de
consola `sm_sw_soak burst` lanza durante 50 segundos a intervalos de 0,5 s:
texto HTTP200, streaming binario con NUL/0xff, HTTP503 esperado y delay de 3 s.
Verifica completion, cuerpo, contexto, chunks sin duplicados, headers y cierre.
No imprime por frame ni por petición: agrega estado/latencias al pedir snapshot,
y limita mensajes de fallo a tres conservando siempre el contador completo.

`sm_sw_soak <fase>` imprime acumulados y tiempos de frame de la ventana desde el
snapshot anterior. `finish` solo desarma la vigilancia de map_end al estar sin
trabajo ni peticiones pendientes, antes del quit autorizado. Una transición de
mapa durante la medición es fallo. OnPluginEnd libera timer y peticiones propias.

Registrar el plan y límites antes de arrancar. Medir reposo/carga/recuperación en
el mismo proceso. Durante cada drenaje, muestrear /proc (RSS/anon/PSS cuando esté
disponible, CPU, FDs, sockets, hilos) y sm_dump_handles. Exportar solo agregados por
tipo/propietario y eliminar el dump crudo. Comparar hashes/mtime de archivos
protegidos, PID/start ticks, módulos y ausencia de logs de errores. No equiparar
RSS de un proceso entero con una prueba de cero fugas de heap del SDK.

La receta aceptada usa tres minutos de reposo, veinte ciclos de 50 s de carga y
10 s de drenaje, y tres minutos de recuperación, tras calentamiento. Umbrales,
muestras y sus límites están en el [informe sostenido](../../docs/runtime-compatibility/soak-srcds-acceptance.md).
No acredita rendimiento de producción, jugadores concurrentes, servicios remotos
ni operación de varias horas. Restaurar DEV con LinuxGSM y verificar sus hashes.

## Cierre humano T25/T29

`steamworks_identity_pending.sp` observa la conversión pública de referencia
serial durante OnClientConnected y compara el mismo cliente después de autorizar.
No altera auth ni memoria. `steamworks_consumer_social.sp` exige licencia prestada,
propietario desconectado para descartar persona de fallback y grupo autorizado.
Configurar clave solo en convar protegida mediante archivo privado temporal,
eliminarlo y no exportar IDs/nombres/tickets. Mantener Connect descargado durante
la entrada humana si el backend de laboratorio no proporciona asignación.
Resultados y límites: [cierre T25/T29](../../docs/runtime-compatibility/last-two-srcds-progress.md).
