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
Los helpers `scripts/tests/helpers/build-{buffer,capability,forward}-fixture.sh`
generan proveedores ELF32 de laboratorio; no se instalan en producto.

## Catálogo de escenarios

| Fuentes | Finalidad y efectos |
|---|---|
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

El GC de cola vacía o el proveedor local no cierran GC remoto. La salida limpia
de un perfil mínimo no cierra el SIGSEGV histórico del baseline completo.
Consulta [cobertura](../COVERAGE.md) para observaciones y pendientes.
