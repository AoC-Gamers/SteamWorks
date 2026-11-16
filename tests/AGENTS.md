# Pruebas

- No debilites assertions ni controles negativos para acomodar movimientos.
- Fixtures críticos mantienen bytes/procedencia y dependencia transitiva; no regeneres P0 con la API actual.
- Separa dobles, SourceHook/VM y SRCDS. Lo no ejecutado queda pendiente.
- `runtime/` requiere operación explícita; ningún ejecutor offline administra servidores.
- Ejecutores en `runners/`, ayudantes en `helpers/`; conserva nombres, CLI y variables. Resuelve raíz desde el archivo, también sin Git y con espacios.
- El saneamiento público de metadatos se documenta con digests anteriores/actuales; no altera medidas binarias ni regenera baselines API/ABI/schema.
