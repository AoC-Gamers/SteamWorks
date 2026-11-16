# Registro de compatibilidad

[P5-R19](p5-integration.md) incorpora REV-GC-01 y REV-HTTP-02. La API pública sigue
igual a P0; el include cambia comentarios de contrato. El ELF `72664c65…` tiene
[aceptación SRCDS propia de alcance GC/HTTP](review12-srcds-acceptance.md).
**Recepción positiva GC remota pendiente; no se declara aceptación global nueva.**

| Superficie | Evidencia del ELF actual |
|---|---|
| Plataforma | Linux L4D2 SRCDS x86; SourceMod/Steam/engine habituales, SourceTVSupport presente |
| API | Firmas, tipos, registros y opcionalidad iguales a P0 |
| HTTP | SteamHTTP real, 36 combinaciones cuerpo/capacidad, vacío/503, contexto/NUL/conteos/canarios |
| Fallo HTTP | Antes del envío conserva salidas en SRCDS; fallos de reserva/lectura completa y rangos extremos son deterministas locales |
| GC | Cola vacía real; proveedor local tipado para éxito/error, hook/VM reales y restauración de target |
| Lifecycle del ensayo | Mismo PID con nuevo juego y quit limpio; no toda la matriz histórica repetida |
| Reversión | Captura por lectura, mismos bytes ZIP R9, roundtrip en filesystem desechable |

R19 repite 15 suites x86 y sanitizadores sin supresiones. Compilan tres
consumidores, 19 harnesses runtime, cuatro fixtures Connect y dos controles de
includes. Sus 66 pruebas Python y build pasan sin modificar sus fuentes.
Los proveedores/includes externos no se distribuyen para forzar recompilación.
Los SMX HTTP de tres argumentos deben recompilarse para el contrato actual.

Auth/identidad/stats humanos, T11 completo, T16 recarga quiescente, T20 descarga
del propietario, T23 multi-consumidores, T25/T29 sociales, T32 recuperación de red,
T33 carga prolongada y T26/T35 en sitio conservan su evidencia histórica en
`839ff075…`; no se atribuyen automáticamente al ELF modificado. La
[matriz conciliada](p5/validation-results.json) identifica procedencia y alcance
actual por caso. [Checkpoint anterior](p5/history/p5-r18-before-review12-integration-validation-results.json).

No se certifican recepción GC remota, otros proveedores, Windows/x64 runtime,
todos los métodos SDK ni ausencia global de fugas/OOM. CI verde de R18 corresponde
a sus commits históricos; no hay CI nuevo para los cambios nativos actuales.
El baseline conserva su fallo conocido al salir. Los paquetes anteriores quedan
intactos. Esta actualización no opera servidores ni hace commit, push o despliegue.
