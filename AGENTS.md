# Trabajo en SteamWorks

- Objetivo de runtime: L4D2 dedicado x86. Conserva la API pública y audita cambios de ABI/SDK.
- Único SDK objetivo: Steamworks 1.55 (`sdk_155`); `sdk_155a` queda descartado. Build, pruebas y CI deben comprobar `scripts/build/steamworks-sdk.lock.json`; no admitir otro release ni inferir aceptación SRCDS a partir del build.
- Lee `docs/README.md`, `docs/build/README.md` y `tests/README.md` antes de editar su ámbito.
- Ejecuta `python3 tests/runners/test-integration.py --output .build/test-results/<nuevo>`; las suites VM/SourceHook son opciones explícitas con dependencias reales.
- No operes servidores, publiques ni despliegues por inferencia de pruebas offline.
- Guarda resultados en `.build/` o `dist/`, no en documentación versionada.
- Archivos instalables bajo `addons/sourcemod/`; include público en `addons/sourcemod/scripting/include/`. Código nativo en `extension/`, diagnósticos en `tests/`.
- No publiques identificadores de instalaciones propias ni rutas locales; usa entradas genéricas y conserva autoría y procedencia técnica.
