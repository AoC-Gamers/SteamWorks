# Harnesses SRCDS: operación explícita

Estas fuentes se conservan para repetir ensayos en un laboratorio autorizado.
Ningún ejecutor offline arranca, detiene ni administra servidores. No ejecutes
una receta histórica por inferencia de un build correcto.

## Preparación

Usa SRCDS L4D2 Linux x86, LinuxGSM, SourceMod/Metamod compatibles y un árbol de
addons aislado. Registra revisión de fuentes, SHA256 ELF/include, módulos/perfil,
lista de consumidores y CFG que deban permanecer intactos. Captura antes los
archivos propios para reversión y separa datos privados de evidencia pública.
No añadas contraseña como condición genérica de una prueba de conexión L4D2.
La cuenta participante y sus escenarios deben estar autorizados.

Compila con `spcomp`, include actual y dependencias explícitas del harness;
no distribuyas includes externos solo para fabricar un paquete autocontenido.
Los helpers `tests/helpers/build-{buffer,capability,forward}-fixture.sh`
generan proveedores ELF32 de laboratorio; no se instalan en producto.

## Catálogo de escenarios

| Fuentes | Finalidad y efectos |
|---|---|
| `steamwork_test.sp` | Diagnóstico manual de estado, HTTP, identidad, licencia, metadata y stats; puede modificar metadata, no instalar como plugin de producto |
| `steamworks_acceptance.sp`, `steamworks_rollback_acceptance.sp` | HTTP, callback/contexto/cierre y comparación tras instalar/revertir |
| `steamworks_stats_acceptance.sp` | Lecturas enteras/float y claves inexistentes con usuario autenticado |
| `steamworks_identity_acceptance.sp`, `steamworks_identity_pending.sp` | Buffers, NUL y negativos antes de autorización |
| `steamworks_connection_observer.sp`, `steamworks_soak_observer.sp` | Observar conexiones y estabilidad sin confundir presencia con autorización |
| `steamworks_consumer_contract.sp`, `steamworks_consumer_observer.sp`, `steamworks_consumer_social.sp` | Consumidores precompilados/recompilados, AccountID/AppID, grupo/perfil y escenarios autorizados |
| `steamworks_legacy_http_consumer.sp` | Rechazo de la llamada textual de tres argumentos |
| `steamworks_pending_owner.sp` | Descarga del propietario con petición pendiente |
| `steamworks_lifecycle_inspect.sp`, `steamworks_lifecycle_pending.sp`, `steamworks_extension_observer.sp` | Estado, carga tardía y descarga/recarga soportada |
| `steamworks_forward_acceptance.sp`, `steamworks_forward_receiver.sp` | Cero, uno y varios receptores con distintos resultados |
| `steamworks_capability_acceptance.sp`, `steamworks_buffer_acceptance.sp` | Capacidades opcionales y límites con proveedor tipado local |
| `steamworks_*_fixture.cpp` y `*_fixture_sdk_config.h` | Proveedores diagnósticos del harness, no runtime de producto |
| `steamworks_gc_observer.sp` | Observación pasiva GC; contadores de envío/disponibilidad/recepción y límites, sin enviar mensajes ni registrar payloads |
| `http_fixture.py` | Endpoint local controlado, revisar puerto antes de arrancar |

Convars y comandos exactos están declarados en cada fuente. Sus flags booleanos
son celdas bool según el include; no intercambiar un byte y una celda por analogía
con C++. No registrar IDs, tickets ni claves Web API; obtener secretos por su
configuración autorizada sin copiarlos a resultados versionados.

## Cierre de un ensayo

Comprueba resultados y logs por comportamiento, cambios de mapa/reentrada cuando
corresponda y ausencia de callbacks al propietario descargado. Para reversión en
sitio detén, aplica el ZIP real capturado y vuelve a comprobar hashes y funciones;
cambiar solo un selector no acredita restauración. Restaura el laboratorio y
verifica configuraciones ajenas antes/después. Un payload idéntico puede citarse
por hash; no extender aceptación a paquetes o consumidores diferentes.

La cola vacía y el proveedor GC local validan contratos de buffers y forwards.
La API y sus controles se conservan por compatibilidad. La salida limpia
de un perfil mínimo no cierra el SIGSEGV histórico del baseline completo.
Consulta [cobertura](../COVERAGE.md) para observaciones y pendientes.

## Observador de consumidores

`steamworks_consumer_observer.sp` no presupone plugins de una instalación propia.
Para comprobar un consumidor que procesa `SteamWorks_SteamServersConnected`,
configúralo antes según su contrato y registra explícitamente cada CFG cuyo
`exec` deba bloquearse durante el ensayo:

```text
sm_sw_consumer protect-config "sourcemod/consumer.cfg"
sm_sw_consumer server-ip "consumer.smx"
```

`server-ip` exige al menos una ruta protegida, comprueba la presencia de los
controles de red del motor, llama al callback real y contrasta la dirección
consumida sin imprimirla; después restaura esos controles. No ajusta convars
propias del consumidor. Los modos `loaded`, `unloaded`, `family` y `absent`
conservan sus escenarios; el modo de callbacks generalizado requiere nueva
aceptación SRCDS antes de atribuirle un resultado runtime.

## Observación de GC remoto

Compila el observador mediante Make; no forma parte del paquete instalable:

```bash
make build-gc-observer SPCOMP=/ruta/a/spcomp \
  SOURCEMOD_DIR=/ruta/a/sourcemod TEST_OUTPUT=.build/gc-observer
```

En una instancia aislada autorizada, carga `steamworks_gc_observer.smx` antes
del escenario de juego y consulta `sm_sw_gc_status` desde la consola del servidor.
Los contadores se conservan durante los cambios de mapa y se reinician al recargar
el plugin. Los dos forwards con resultado devuelven OK (cero), conservando el
resultado original; el observador no inicia envíos ni consume la cola por su cuenta.
La salida se limita a 20 eventos por forward; el comando informa los totales.

`connected=1` solo acredita conexión del gameserver a Steam. Cero mensajes no
acredita recepción remota ni ausencia permanente de backend. Un snapshot no vacío
requiere contrastar el proveedor, consumidores y escenario antes de atribuirlo a
Steam; no cargar fixtures de inyección en esta prueba. Conservar hashes del ELF,
include y módulos cargados, ventana de observación y resultado de cada escenario.
No extrapolar símbolos GC presentes en un módulo a una ruta activa del dedicado:
verificar el módulo realmente mapeado y la implementación de sus puntos de entrada.

Para distinguir interfaz disponible de cola vacía, el observador ofrece
`sm_sw_gc_interface <getter-hex>`, solo desde consola. Antes de usarlo, el host
debe verificar SHA256 ELF, símbolo local `GetSteamGCPointer()` sin argumentos,
su ABI y dirección ejecutable a partir del módulo actualmente mapeado. No
reutilizar direcciones entre procesos ni revisiones. El sondeo llama al getter y,
si devuelve una interfaz, a `IsMessageAvailable(uint32*)` en el slot 1 auditado de
SteamGameCoordinator001 del SDK 1.55. Informa disponibilidad/tamaño sin recuperar
ni enviar mensajes. Los handles SDKCall se cierran inmediatamente.

## Compilar consumidores con el include actual

```bash
make build-runtime-harnesses SPCOMP=/ruta/a/spcomp \
  RUNTIME_EXTRA_INCLUDE=/ruta/a/family-share/include \
  TEST_OUTPUT=.build/runtime-consumers
```

El target compila todos los harnesses con warnings tratados como errores y genera
el include de claves desde el schema protegido. No instala ni carga plugins.
`steamworks_stats_callbacks.sp` observa los dos forwards nuevos;
`sw_stats_callbacks_status` muestra solo contadores de recepción, fallos y descarga
de caché. `sw_stats_callbacks_test`, ejecutado explícitamente desde la consola,
requiere exactamente un humano autorizado. Solicita stats por cliente y AccountID
con un duplicado pendiente; comprueba un único resultado, la correspondencia de
identidad/AppID y lecturas enteras coincidentes después de `k_EResultOK`. Luego
repite la solicitud para comprobar una nueva notificación. Un timeout, una
identidad distinta, una lectura fallida o notificaciones extra impiden el PASS.
Los cambios de mapa y la desconexión abortan el ensayo activo, pero conservan
el AccountID observado para poder reconocer una descarga posterior de caché.
Ese objetivo se sustituye al aceptar un nuevo ensayo o se borra con
`sw_stats_callbacks_reset`. El estado muestra `watching` y `matched_unloaded`;
no registra identidades ni valores personales de stats. La descarga de caché solo
se observa si Steam la emite; no se simula para atribuir recepción runtime.
La recepción real de `OnUserStatsUnloaded` está excluida de la aceptación global
de L4D2 dedicado x86 y no bloquea el PASS del ensayo de resultados. El observador
permanece disponible para investigación; la exclusión no acredita el evento.
Se mantienen las pruebas deterministas y sus controles negativos. Consulta el
[límite funcional](../../docs/stats/README.md#descarga-de-caché-en-l4d2-dedicado-x86).
Su compilación no equivale a aceptación en SRCDS.

Para una recarga manual, retira primero los plugins dependientes y utiliza la
identidad lógica `sm exts load steamworks`. El nombre físico del artefacto no
sustituye esa identidad. No fuerces la descarga con dependientes ni cuentes una
carga con otro nombre como una recarga soportada.

## Propiedad de carga y doble nombre

`steamworks_load_probe.sp` declara solo los natives necesarios y omite
intencionadamente el descriptor `SharedExtension`: permite probar al propietario
cargado por nombre físico sin solicitar otra carga lógica. Es un harness, no un
patrón de dependencia recomendado para plugins de producto. El consumidor
`steamworks_rollback_acceptance.sp`, con el include normal, comprueba por separado
la resolución de la dependencia lógica y su recuperación tras un intento fallido.

`sw_load_probe hold` envía una petición con contexto a `/delay`; `sw_load_probe`
comprueba conexión, identidad de la ConVar y contadores. La finalización valida
el cuerpo y el contexto sin registrar datos de usuarios. Para el endpoint:

```bash
make serve-runtime-http-fixture TEST_OUTPUT=.build/http-counts.json
```

El target ocupa solo loopback en 27880; revisa que esté libre. Ctrl+C lo detiene
y guarda los conteos. El backend conserva su modo HTTP+TLS cuando se proporcionan
certificado y clave; `--http-only` es el modo utilizado por este target. Ninguna
prueba local arranca SRCDS ni instala el harness como efecto secundario.
