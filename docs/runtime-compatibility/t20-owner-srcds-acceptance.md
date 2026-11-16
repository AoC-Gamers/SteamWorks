# T20: descarga del propietario con HTTP pendiente en SRCDS x86

2026-11-15. **T20 pasa en el alcance ejecutado**, con el ELF optimizado de P5-R10
`839ff0751b04141031bdc6dffc48662d0804f52227f2c91015bd87c6b84cf192` extraído del
ZIP R10. La extensión permaneció cargada durante las descargas del plugin. API
pública y fuentes de extensión intactas; `develop`, sin commit ni producción.

## Entorno y protocolo

Se usó `competitive_dev`, LinuxGSM `/app/l4d2server`, puerto 27015 y una copia en
`addons/sourcemod_acceptance`, con flags/configs habituales y sin añadir contraseña.
El módulo realmente mapeado se verificó por dispositivo/inodo/hash. El PID
**99963**, sus ticks de inicio y los módulos seleccionados se conservaron antes y
después del ensayo, incluido SourceTVSupport. No se sustituyeron VM ni bibliotecas.

El harness `steamworks_pending_owner.sp` deja de depender de un retraso fijo y
registra un comando controlado. `/hold/<secuencia>` no envía headers/cuerpo hasta
que el runner libera su compuerta. Cada petición tiene timeout de 60 segundos y
contexto sintético de dos celdas. El fixture solo escucha en loopback y no registra
headers, cuerpos recibidos, IDs de usuarios ni credenciales.

Primero se libera una respuesta con el propietario cargado: el callback recibe
HTTP 200, cuerpo/contexto correctos y cierra su handle dentro del callback. Este
control positivo confirma que la ruta de callbacks funciona.

Después se ejecutan **dos ciclos**:

1. Enviar una petición retenida y comprobar que la fixture ya la recibió, sin
   liberar la respuesta ni alcanzar su timeout.
2. Comprobar un `HTTPHandle` del propietario y descargar el plugin. `OnPluginEnd`
   observa la petición pendiente; deliberadamente no ejecuta `delete`, para probar
   el listener de la extensión y la limpieza automática de handles de SourceMod.
3. Confirmar que el plugin está ausente y su conteo de `HTTPHandle` es cero.
4. Recargar el mismo nombre de plugin antes de liberar la respuesta antigua.
   Una petición nueva debe completar y cerrar correctamente.
5. Liberar la respuesta antigua y observar siete segundos: ningún callback de su
   secuencia, ningún callback adicional al nuevo plugin ni errores/crash.
6. Confirmar otra petición nueva correcta después de la liberación y handle cerrado.

## Recursos y resultados

| Punto de cada ciclo | Plugin propietario | HTTPHandle del propietario |
|---|---|---:|
| Antes de crear | Cargado | 0 |
| Respuesta retenida | Cargado | 1 |
| Después de descargar | Ausente | 0 |
| Recargado / probes completados | Cargado | 0 |

Se filtran dentro del guest los dumps de `sm_dump_handles` por propietario y tipo.
El runner usa un comando posterior como barrera de finalización del dump y elimina
inmediatamente el archivo crudo. Solo exporta filas/conteos del propietario de
prueba. Su tamaño aproximado era 136 bytes por handle observado; esto no mide
memoria global ni todas las reservas internas de Steam.

Pasan el control positivo retenido y cuatro probes nuevos, con cierre dentro de
sus callbacks. Las dos respuestas antiguas no generan callbacks. La fixture
registra ocho peticiones contando el smoke HTTP inicial. No hay errores SourceMod,
filas de plugins fallidos, marcadores de crash ni nuevo core del candidato. El
quit final es limpio. Los harnesses propietario/smoke compilan sin warnings contra
el include extraído del ZIP R10.

La prueba acepta reclamación de handles visibles del propietario y ausencia de
callbacks obsoletos durante las ventanas medidas. **No acredita ausencia global
de fugas de heap/SDK, operación sostenida ni descarga en caliente de la extensión**:
T33 y T16 conservan sus pendientes. No se repiten autenticación, identidad o stats.

## Restauración y atribución

LinuxGSM arrancó el DEV habitual, PID **102174**, con selector/módulo originales y
Steam conectado. Se verificaron los cuatro hashes protegidos; laboratorio,
recovery privado y fixture se retiraron. Los cores originales de systemd se
conservan; no se exportaron cores, argumentos ni entorno.

Solo cambió `tests/runtime/steamworks_pending_owner.sp`. La matriz registra sus
hashes previo/empaquetado y actual, conservando la
[matriz anterior](p5/history/p5-r10-before-T20-owner-acceptance-validation-results.json).
Los ZIP R10 permanecen intactos y contienen su snapshot de fuentes anterior al
nuevo harness. No se regenera una entrega P5 durante esta aceptación.

[Resultados/hashes](t20-owner-srcds/results.json),
[control positivo retenido](t20-owner-srcds/positive-held.json),
[callback positivo](t20-owner-srcds/positive-completed.json),
[descarga 1](t20-owner-srcds/unloaded-1.json),
[recarga antes de liberar](t20-owner-srcds/reloaded-1.json),
[observación 1](t20-owner-srcds/released-observation-1.json),
[descarga 2](t20-owner-srcds/unloaded-2.json),
[observación 2](t20-owner-srcds/released-observation-2.json),
[módulos finales](t20-owner-srcds/final-modules.json),
[salida](t20-owner-srcds/shutdown.json),
[auditoría de cores](t20-owner-srcds/core-audit.json) y
[restauración](t20-owner-srcds/cleanup.json).

Paquete separado de revisión:
`.build/t20-owner-srcds/review/steamworks-t20-pending-owner-linux-x86-srcds-acceptance.zip`,
con manifiesto/checksums. Incluye evidencia filtrada, harness y runner/fixture;
excluye configuraciones privadas, dumps crudos, logs de LinuxGSM, SDK, módulos del
juego y cores. P5 mantiene ocho casos abiertos y no tiene aceptación global.
