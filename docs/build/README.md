# Compilación y empaquetado

El build de producto usa AMBuild y C++ con las declaraciones auditadas del SDK
Steamworks 1.55. No actualices el SDK como efecto de reorganizar documentación.
Consulta las [dependencias](dependencies.md) y la separación de [CI](ci.md).

Desde la raíz de las fuentes, también extraídas sin `.git`:

```bash
make deps-exts-linux
make build-exts-linux STEAMWORKS_SDK_DIR=/ruta/al/sdk_155
make package-exts-linux
make release-linux
```

En Windows usa los targets `deps-exts-windows`, `build-exts-windows`,
`package-exts-windows` y `release-windows`, con PowerShell y el toolchain x86
configurado. Los targets `release-*` generan ZIP locales; la publicación pertenece
a un flujo separado. `make help` describe todos los targets existentes.
Los scripts también resuelven su raíz desde su ubicación, sin depender del cwd.

Los scripts de build seleccionan `--sdks l4d2 --target x86`. Para una
compilación optimizada añade `--enable-optimize` al configurar directamente con
`configure.py` y ejecuta AMBuild desde ese directorio de build.
El build optimizado mantiene comprobaciones y `-Werror`; los tests del backup
CDetour y de la asignación de autenticación también se compilan con optimización
para detectar las regresiones que aparecieron con `--enable-optimize`.

`DEPS_DIR`, `BUILD_DIR`, `STEAMWORKS_SDK_DIR` y `STEAMWORKS_SDK_NAME` permiten
seleccionar rutas. El Makefile separa directorios Linux/Windows; los scripts
admiten `.env` local. No publiques credenciales ni dependas de una ruta privada.
La opción `SOURCE_DATE_EPOCH` fija la fecha de compilación para reproducibilidad;
no es la fecha de ejecución de las pruebas.

Las fuentes instalables siguen `addons/sourcemod/`: el include público vive en
`addons/sourcemod/scripting/include/steamworks.inc`. El código C++ permanece en
`extension/` y AMBuild genera el binario en `package/addons/sourcemod/extensions/`.
El binario distribuido se llama `steamworks.ext.2.l4d2.so` en Linux y
`steamworks.ext.2.l4d2.dll` en Windows; el include conserva `steamworks.ext` como
identidad lógica. Usa una salida de build nueva al migrar desde el nombre genérico.
Los harnesses de diagnóstico viven en `tests/runtime/` y no se empaquetan.

El mapa `scripts/package/package-map.json` es una entrada de validación del
empaquetado y permanece solo en las fuentes. El staging admite únicamente
extensión/include, README, CHANGELOG, LICENSE y Markdown estable de
`docs/`, excluyendo instrucciones AGENTS. El validador rechaza SDK, tests,
fixtures, gamedata inventado, binarios de diagnóstico y logs. Los resultados de
validación se guardan en `.build/` y los paquetes en `dist/`, fuera de Git.
Compilar Windows no equivale a aceptar su ejecución en SRCDS.

## Selección única del SDK

`scripts/build/steamworks-sdk.lock.json`, en las fuentes, fija 45 headers y dos bibliotecas x86
(Linux y Windows). `scripts/build/validate-steamworks-sdk.py` comprueba el conjunto
de headers y cada SHA256. Los headers normalizan únicamente CRLF a LF para
comparar Linux/Windows y Git; las bibliotecas conservan comprobación byte por byte. AMBuild invoca esa comprobación antes de configurar el
producto, tanto desde scripts como con `configure.py` directo. Los ejecutores
que consumen SDK y la integración también la exigen.

Una ruta alternativa mediante `STEAMWORKS_SDK_DIR` debe contener exactamente el
release 1.55. Cambiar el nombre de carpeta no habilita otro release. El selector
manual de CI solo admite `sdk_155`. El SDK y su lock son entradas de build; el
paquete instalable no incluye headers/bibliotecas del SDK ni el lock de fuentes.
