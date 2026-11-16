# Registro de compatibilidad

[P5-R17](p5-integration.md) integra el cierre T25/T29 de R16 y conserva las
aceptaciones previas por hash y alcance. [Matriz T01–T36](p5/validation-results.json):
33 casos aceptados, tres no aplicables (T13/T14/T34), ninguno pendiente en el perfil.

| Superficie | Alcance aceptado |
|---|---|
| Plataforma | Linux L4D2 SRCDS x86, engine/Steam/SourceMod identificados por hash |
| API pública | Igual a P0 en firmas, tipos, registros y opcionalidad; include `a3e5a7a2…` |
| ELF | Optimizado `839ff075…`, reproducido con fecha de compilación fijada |
| Auth/identidad/stats | Adaptador 014, forwards, cuenta real, lectura de stats, salidas fallidas preservadas, buffers y negativos |
| T29 | Entrada real pendiente→autorizada; bot/desconectado anteriores; fallback/contradicción deterministas e invariante del proveedor real |
| T25 | Consumidores existentes/recompilados y H08/H09; préstamo, perfil Web API, grupo; Connect corregido en frío/HTTP/ACK/mapa |
| T11 | Ocho interfaces proporcionadas; Matchmaking ausente en gameserver; GC local y rechazo de versión desconocida |
| Lifecycle | T08 reinicialización con SourceTVSupport, T16 carga tardía/recarga quiescente, T20 propietario HTTP pendiente |
| Forwards | T23: cero/uno/varios consumidores y resultados en SourceMod real con argumentos sintéticos |
| T32/T33 | Un ciclo real de recuperación Steam; veinte minutos de carga local y límites medidos |
| Instalación/reversión | T26/T35 previos en sitio; R17 repite roundtrip local y conserva mismo ZIP rollback de R9 |

R17 repite 15 suites x86 y sanitizadores sin supresiones, recompila tres consumidores,
18 harnesses runtime, cuatro fixtures Connect y dos controles de includes. Las
66 pruebas Python de Connect pasan. Inventario actualizado: cero llamadas fuera
de aridad declarada. Los SMX antiguos que usan tres argumentos HTTP siguen siendo
incompatibles y requieren recompilación; no se modifica el proveedor para aceptarlos.
El parche de Connect se entrega fuera del payload instalable de SteamWorks.

No se infieren Windows/x64, otros proveedores/versiones, todos los métodos SDK,
GC/Core remoto, partidas completas ni capacidad durante varias horas. T16 no
incluye descarga forzada en callbacks activos; T20 no demuestra ausencia universal
de fugas SDK. El baseline conserva su fallo conocido al salir.

Documentación y evidencia histórica se conservan: [estado anterior](p5/history/p5-r16-with-T25-T29-closed-compatibility.md),
[cierre T25/T29](last-two-srcds-progress.md), [T32](connection-srcds-acceptance.md),
[T33](soak-srcds-acceptance.md). Los ZIP R16 no se modifican. R17 no opera servidores,
publica ni despliega. Todo permanece en develop, sin commit.
