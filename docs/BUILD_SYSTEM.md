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

## Manifiesto

[plugin-package-map.json](../plugin-package-map.json) deja explícito:

- qué extensión forma parte del bundle
- qué archivos runtime deben acompañarla

En este repo, el paquete final incluye:

- `addons/sourcemod/extensions/steamworks.ext.so` o `steamworks.ext.dll`
- `addons/sourcemod/scripting/include/steamworks.inc`

## CI

El workflow principal separa:

- `deps-exts-linux`
- `build-exts-linux`
- `release-linux`
- `deps-exts-windows`
- `build-exts-windows`
- `release-windows`
- `publish`

Los jobs `release-*` absorben el empaquetado liviano antes de generar el ZIP final.
