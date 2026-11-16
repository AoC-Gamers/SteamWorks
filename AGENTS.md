# Trabajo en SteamWorks

- Objetivo de runtime: L4D2 dedicado x86. Conserva la API pública y audita cambios de ABI/SDK.
- Lee `docs/README.md`, `docs/build/README.md` y `tests/README.md` antes de editar su ámbito.
- Ejecuta `python3 scripts/tests/test-integration.py --output .build/test-results/<nuevo>`; las suites VM/SourceHook son opciones explícitas con dependencias reales.
- No operes servidores, publiques ni despliegues por inferencia de pruebas offline.
- Guarda resultados en `.build/` o `dist/`, no en documentación versionada.
