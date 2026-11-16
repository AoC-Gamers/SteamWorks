# Build System

Este repositorio usa un flujo nativo puro para construir la extensión de SteamWorks.

## Objetivo

El mismo esquema debe servir para:

- compilación local Linux
- compilación local Windows
- compilación en GitHub Actions

La fuente real del build nativo sigue siendo `AMBuild`, pero `make` actúa como interfaz corta y el CI reutiliza esos mismos targets.

## Targets

Linux:

- `make deps-exts-linux`
- `make build-exts-linux`
- `make package-exts-linux`
- `make release-linux`

Windows:

- `make deps-exts-windows`
- `make build-exts-windows`
- `make package-exts-windows`
- `make release-windows`

## Separación por plataforma

Las dependencias se separan para no mezclar toolchains:

- `.deps/exts-linux`
- `.deps/exts-windows`

Los outputs también se separan:

- `.build/linux-l4d2`
- `.build/windows-l4d2`
- `.build/package-exts-linux`
- `.build/package-exts-windows`
- `dist/linux/artifact`
- `dist/windows/artifact`

Los ZIP finales se escriben en `dist/release/`. Por defecto, sus nombres son
`steamworks-local-linux.zip` y `steamworks-local-windows.zip`; `RELEASE_BASENAME`
permite cambiar el nombre en cada target `release-*`.

## Manifiesto

[plugin-package-map.json](../plugin-package-map.json) deja explícito:

- qué extensión forma parte del bundle
- qué archivos runtime deben acompañarla

En este repo, el paquete final incluye:

- `addons/sourcemod/extensions/steamworks.ext.so` o `steamworks.ext.dll`
- `addons/sourcemod/scripting/include/steamworks.inc`

AMBuild genera esos archivos bajo `.build/<plataforma>-l4d2/package/`.
Los targets `package-exts-*` y `release-*` usan
[scripts/stage-artifact.py](../scripts/stage-artifact.py) para añadir también
`README.md`, `CHANGELOG.md`, `LICENSE`, `plugin-package-map.json` y `docs/`.
El plugin de pruebas `scripting/steamwork_test.sp` queda fuera del paquete.
El gamedata opcional `steamworks.txt` no está incluido en el repositorio ni se
empaqueta.

Para preparar candidatos de revisión y reversión sin publicación, consultar
[P5](runtime-compatibility/p5-integration.md). Sus ZIP locales tienen manifiesto
y checksums, y excluyen SDK, configuración del servidor y plugins de prueba.
P5-R15 compila el candidato con `configure.py --enable-optimize --target x86`
y reproduce el ELF optimizado aceptado en SRCDS. La receta exacta acompaña al ZIP
de fuentes en `review/run-build.sh`; el helper general de build conserva sus
opciones predeterminadas. Los diagnósticos SourcePawn instrumentados van separados
del instalable y las fuentes incluyen los parches, sin dependencias del SDK.
El flujo general de Windows conserva herramientas de compilación; no implica
un perfil interno aceptado para ese sistema.

## CI

El [workflow principal](../.github/workflows/main.yml) separa:

- `deps-exts-linux`
- `build-exts-linux`
- `release-linux`
- `deps-exts-windows`
- `build-exts-windows`
- `release-windows`
- `publish`

Los jobs `release-*` preparan los paquetes, generan los ZIP y ejecutan
[scripts/ci-validate-artifact.sh](../scripts/ci-validate-artifact.sh).
El job `publish` depende de ambos builds y ambos releases, y publica los ZIP de
Linux y Windows. Los canales son `channel/latest` para `main` y
`channel/develop` para `develop`; los tags `v*` generan releases versionados.
