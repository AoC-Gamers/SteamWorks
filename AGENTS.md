# Trabajo en SteamWorks

Estas instrucciones cubren todo el repositorio; no mantener copias por directorio.
Antes de editar, consulta `docs/README.md`, `docs/build/README.md` y `tests/README.md`.

## Contratos que deben conservarse

- Objetivo runtime: L4D2 dedicado x86. Conserva API pública, firmas, forwards y
  opcionalidad; audita cambios de ABI/SDK.
- Único SDK: Steamworks 1.55 (`sdk_155`). Build, pruebas y CI verifican
  `scripts/build/steamworks-sdk.lock.json`; no aceptar otro release por su nombre.
- No debilitar assertions, sanitizadores ni controles negativos. No regenerar el
  inventario original de API, schemas o fixtures para hacer pasar una comparación.
- Conserva bytes, procedencia y hashes transitivos de fixtures. El saneamiento
  público de metadatos no cambia medidas binarias ni flags de aceptación.
- Compilación, dobles, SourceHook/VM y SRCDS son niveles distintos de evidencia.
  Lo no ejecutado no cuenta como aprobado. GC remoto está excluido de la aceptación
  global de L4D2 dedicado; conservar API y pruebas locales por compatibilidad, sin
  atribuir recepción remota positiva ni retirar comprobaciones.

## Mapa de rutas

| Ruta | Responsabilidad |
|---|---|
| `addons/sourcemod/scripting/include/` | API pública instalable |
| `extension/` | Código nativo y backend AMBuild |
| `scripts/build/`, `scripts/deps/` | Compilación, lock SDK y dependencias |
| `scripts/package/`, `scripts/ci/` | Mapa, staging, ZIP y validación de producto |
| `scripts/tools/` | Inventario y comprobaciones documentales/binarias |
| `tests/runners/`, `tests/helpers/` | Ejecutores internos y ayudantes |
| `tests/native/`, `tests/integration/`, `tests/python/` | Regresiones offline |
| `tests/fixtures/` | Entradas protegidas y procedencia |
| `tests/runtime/`, `tests/diagnostics/` | Ensayos SRCDS y sondeos explícitos |
| `docs/<feature>/` | Contratos canónicos en español, rutas kebab-case |
| `.build/`, `dist/`, `.deps/` | Resultados, paquetes y dependencias locales; fuera de Git |

## Ejecución obligatoria mediante Make

Makefile es la interfaz de ejecución del proyecto, tanto localmente como en CI.
No documentes ni ejecutes scripts Python/shell o AMBuild como rutas públicas
alternativas: los targets Make delegan en esos backends y pueden invocarlos
internamente. CI prepara host/checkouts/cachés y coordina la publicación autorizada;
las pruebas, build, paquetes y validación de producto siempre pasan por Make. Conserva nombres, CLI y variables de los scripts internos.

Desde la raíz:

```bash
make help
make validate-sdk STEAMWORKS_SDK_DIR=/ruta/al/sdk_155
make test TEST_OUTPUT=.build/test-results/nueva-ejecucion
make test-public-ip
make check-docs
```

- `make test` compara toda la API con el inventario original y ejecuta la
  integración x86. `TEST_OUTPUT` debe ser una carpeta nueva; se rechaza reutilizarla.
- `make test-<suite>` ejecuta una suite focalizada. Selecciona lo apropiado al cambio;
  no conviertas un ajuste documental en una obligación de ejecutar SRCDS.
- `make test-ci` reúne las suites de CI, incluida SourceHook. La documentación
  sigue siendo una comprobación local; no añadir de nuevo un job documental.
- `TEST_OPTIONS` permite seleccionar `--with-auth-binding`, `--with-auth-forward`
  y `--with-sourcepawn-environment` en la integración; requieren dependencias reales.
  Consultar `tests/integration/README.md`; no omitir silenciosamente dependencias.
- `make test-sdk-incremental TEST_OUTPUT=<nueva-carpeta>` requiere AMBuild,
  SourceMod, Metamod y HL2SDK reales; comprueba el build optimizado y su guarda SDK.
- `TEST_CXXFLAGS` usa `-m32` por defecto. Mantén los flags de sanitizer de los
  ejecutores; conserva `VM_TEST_CXX`/`VM_TEST_FLAGS` para la VM instrumentada.
- `make deps-exts-linux`/`deps-exts-windows` descarga dependencias explícitamente.
  Las pruebas no deben descargarlas ni administrar servidores como efecto secundario.
- Build/paquetes: `make build-exts-linux`, `package-exts-linux`, `release-linux`,
  `validate-artifact-linux`; equivalentes `windows`. Preparar un ZIP no lo publica.

Fuera de la raíz usa `make -f "/ruta/con espacios/SteamWorks/Makefile" <target>`.
Las rutas de herramientas se resuelven desde Makefile; una salida relativa indicada
por el usuario se interpreta desde su cwd. Debe funcionar también sin `.git`.

## Documentación y entrega

- Sigue `docs/maintenance/documentation.md`: explica etiquetas históricas y
  límites; no recrees diarios internos ni publiques entornos, rutas o secretos propios.
- Comprueba consumidores, cachés e imports antes de mover archivos. No empaquetar
  SDK, herramientas o fixtures como producto; respetar ownership de archivos.
- Informa comandos Make, resultados, alcance y pendientes. No atribuir aceptación
  runtime nueva a cambios editoriales, paquetes regenerados o pruebas offline.
- No operar servidores, publicar ni desplegar sin autorización de la tarea.
