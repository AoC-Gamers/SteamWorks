# Compilación y empaquetado

El build de producto usa AMBuild y C++ con las declaraciones auditadas del SDK
Steamworks 1.58a. No actualices el SDK como efecto de reorganizar documentación.
Consulta las [dependencias](dependencies.md) y la separación de [CI](ci.md).

Desde la raíz de las fuentes, también extraídas sin `.git`:

```bash
make deps-exts-linux
make build-exts-linux STEAMWORKS_SDK_DIR=/ruta/al/sdk_158a
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

El staging admite solo extensión/include, activos raíz y Markdown estable de
`docs/`, excluyendo instrucciones AGENTS. El validador rechaza SDK, tests,
fixtures, gamedata inventado, binarios de diagnóstico y logs. Los resultados de
validación se guardan en `.build/` y los paquetes en `dist/`, fuera de Git.
Compilar Windows no equivale a aceptar su ejecución en SRCDS.
