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
make build-exts-linux STEAMWORKS_SDK_NAME=sdk_158a
make package-exts-linux
make release-linux
```

El ejemplo usa el SDK objetivo `1.58a` ubicado en `sdk_158a/`. Sin un override,
`make build-exts-linux` busca el directorio `sdk/`. También se puede indicar
`STEAMWORKS_SDK_DIR` con una ruta absoluta.

En Windows:

```powershell
make deps-exts-windows
make build-exts-windows STEAMWORKS_SDK_NAME=sdk_158a
make package-exts-windows
make release-windows
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
- `scripting/include/steamworks.inc`: API pública SourcePawn
- `scripting/steamwork_test.sp`: plugin local de pruebas
- `sdk/`, `sdk_155/`, `sdk_158a/`: posibles copias locales del SDK de SteamWorks, no versionadas

## Convenciones

- `.deps/exts-linux/` y `.deps/exts-windows/` contienen las dependencias por plataforma y no se versionan
- `.build/` no se versiona
- `dist/` contiene los artefactos de release y no se versiona
- `.env` es opcional y sirve para overrides locales; ver [variables y precedencia](../README.md#variables-de-entorno)
- el gamedata `addons/sourcemod/gamedata/steamworks.txt` es opcional y debe
  proporcionarse por separado si se necesitan overrides; no se distribuye en el repo
- la via oficial de build es `Makefile` + scripts
- `make deps-exts-*` no descarga el Steamworks SDK; ese contenido debe existir ya en
  `sdk`, `sdk_155`, `sdk_158a` u otra ruta local equivalente

## Verificacion recomendada

1. validar el nombre o ruta efectiva de `STEAMWORKS_SDK_*`
2. confirmar que el target `make build-exts-*` de la plataforma sigue compilando
3. ejecutar los targets `package-exts-*` y `release-*`
4. validar el artefacto Linux con:

   ```bash
   EXTS_ARTIFACT_DIR="$PWD/dist/linux/artifact" EXTS_PLATFORM=linux bash scripts/ci-validate-artifact.sh
   ```

   Para Windows, usar `dist/windows/artifact` y `EXTS_PLATFORM=windows` desde Bash.
5. probar en servidor las rutas afectadas usando el plugin de pruebas;
   ver [cobertura runtime](TestCoverage.md). La compilación y la validación del
   paquete no sustituyen esa comprobación.
