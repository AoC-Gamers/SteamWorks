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

GC remoto está excluido de la aceptación global de L4D2 dedicado; la recepción
remota no está acreditada. La cola vacía y el proveedor local solo validan
contratos locales. La API y sus controles se conservan por compatibilidad. La salida limpia
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
