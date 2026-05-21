# Desarrollo local

## Objetivo

Este repositorio usa un flujo reproducible basado en:

- `AMBuild`
- `Makefile`
- scripts en `scripts/`
- dependencias locales en `.deps/`
- artefactos de compilacion en `.build/`

La idea es evitar rutas editadas a mano y mantener un proceso local parecido al
de CI.

## Comandos principales

```bash
make help
make deps-exts-linux
make build-exts-linux
```

Si usas una version distinta del SDK de SteamWorks:

```bash
make build-exts-linux STEAMWORKS_SDK_NAME=sdk_158a
```

En Windows:

```powershell
make deps-exts-windows
make build-exts-windows
```

## Estructura relevante

- `AMBuildScript`: configuracion general del build
- `extension/AMBuilder`: lista de fuentes nativas
- `extension/core/`: integracion base, gamedata, forwards y utilidades
- `extension/natives/`: natives SourcePawn
- `extension/hooks/`: hooks y detours
- `extension/http/`: capa HTTP
- `extension/sdk/`: bootstrap base del SDK de SourceMod
- `extension/steamtools/`: parsing auxiliar de blobs y tickets
- `extension/CDetour/`, `extension/asm/`: dependencias nativas auxiliares
- `scripts/`: bootstrap y build local
- `scripting/`: include SourcePawn
- `scripts/steamwork_test.sp`: plugin local de pruebas
- `sdk`, `sdk_155`, `sdk_158a`: copias locales del SDK de SteamWorks

## Convenciones

- `.deps/` no se versiona
- `.build/` no se versiona
- `.env` es opcional y sirve para overrides locales
- la via oficial de build es `Makefile` + scripts
- `make deps-exts-*` no descarga el Steamworks SDK; ese contenido debe existir ya en
  `sdk`, `sdk_155`, `sdk_158a` u otra ruta local equivalente

## Verificacion recomendada

1. confirmar que `make build-exts-linux` sigue compilando
2. revisar que el paquete final contenga `steamworks.ext.so` y `steamworks.inc`
3. si cambias de SDK, validar el nombre o ruta efectiva de `STEAMWORKS_SDK_*`
