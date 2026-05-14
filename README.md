SteamWorks
==========

Extensión para exponer funciones de SteamWorks a SourcePawn.

Solo Linux 32-bit
-----------------

Este repositorio se mantiene principalmente para la compilación Linux de 32
bits usada por servidores dedicados de Left 4 Dead 2.

El flujo principal, el empaquetado y las releases del proyecto están
orientados a Linux 32-bit. También existe una compilación Windows como job
separado dentro del workflow principal, pero su artefacto no se publica en los
releases oficiales.

Compilacion Local
-----------------

### Linux

Comandos:

```bash
make deps-linux
make build-linux
make build-linux STEAMWORKS_SDK_NAME=sdk_158a
```

### Windows

Comandos:

```powershell
make deps-windows
make build-windows
```

Para compilar en Windows necesitas una instalacion de Visual Studio / Build
Tools con soporte C++ x86/x64. Si `cl.exe` no esta disponible en el entorno
actual, el script intenta localizar `vcvarsall.bat` automaticamente.

SDK de SteamWorks
-----------------

La versión objetivo usada por este proyecto es Steamworks SDK `1.58a`.

Importante:

- los scripts `make deps-linux` y `make deps-windows` no descargan
  el Steamworks SDK
- el SDK de SteamWorks debe colocarse manualmente en el repo
- ejemplos de directorios locales validos:
  - `sdk`
  - `sdk_155`
  - `sdk_158a`

El flujo asume que la version que quieras usar ya existe localmente y que luego
la seleccionas con `STEAMWORKS_SDK_NAME` o `STEAMWORKS_SDK_DIR`.

En CI, esa version se obtiene desde el repositorio:

- `AoC-Gamers/Steamworks-SDK`
- directorio `sdk_158a`

Requisitos
----------

- `bash`
- `git`
- `python3`
- un toolchain Linux de 32 bits funcional

Dependencias de compilación en Linux
------------------------------------

En un entorno tipo Ubuntu/Debian, el flujo de CI instala al menos:

- `python3`
- `python3-venv`
- `make`
- `gcc-multilib`
- `g++-multilib`
- `clang`
- `zip`

El objetivo es generar una extensión ELF de 32 bits para SourceMod.

Flujo de compilación
--------------------

Este repositorio incluye scripts auxiliares para descargar las dependencias de
compilación y luego construir la extensión con AMBuild.

Los scripts descargan:

- `hl2sdk`
- `sourcemod`
- `metamod-source`
- `ambuild`

Pero no descargan el Steamworks SDK.

Comandos:

```bash
make deps-linux
make build-linux
make build-linux STEAMWORKS_SDK_NAME=sdk_158a
```

Artefacto local:

- `.build/linux-l4d2/package/addons/sourcemod/extensions/steamworks.ext.so`

Estructura del código nativo
----------------------------

El código bajo `extension/` quedó agrupado por responsabilidad:

- `extension/core/`: integración base de SteamWorks, forwards, gamedata y utilidades internas
- `extension/natives/`: natives expuestos a SourcePawn
- `extension/hooks/`: hooks y detours contra GameServer / GameCoordinator
- `extension/http/`: capa HTTP y requests
- `extension/sdk/`: bootstrap base del SDK de SourceMod
- `extension/steamtools/`: parsing auxiliar de blobs y tickets
- `extension/CDetour/` y `extension/asm/`: dependencias nativas auxiliares

Dependencias en runtime dentro del gameserver
---------------------------------------------

Para que la extensión pueda cargar correctamente en el servidor, el entorno de
Left 4 Dead 2 debe cumplir al menos con esto:

- SourceMod y Metamod:Source para Linux x86
- un servidor dedicado de Left 4 Dead 2 en Linux
- `bin/libsteam_api.so` disponible dentro de la instalación del servidor

En la build Linux, la extensión se enlaza con:

- `libsteam_api.so`

Y se genera con un `rpath` que apunta a:

- `$ORIGIN/../../../../bin`

Eso significa que, al cargarse desde:

- `addons/sourcemod/extensions/steamworks.ext.so`

la librería esperada se resuelve como:

- `bin/libsteam_api.so`

relativa a la raíz del juego.

En una instalación típica de L4D2 dedicada, la ruta efectiva será:

- `left4dead2/bin/libsteam_api.so`

Si esa librería no existe, o si el entorno no puede cargar binarios Linux de 32
bits, la extensión no iniciará correctamente en el gameserver.

Flujo de CI
-----------

GitHub Actions usa jobs separados para:

- preparar dependencias de compilación
- construir Linux 32-bit
- construir Windows x86
- publicar releases oficiales usando solo el artefacto Linux

El workflow principal hace lo siguiente:

- clona este repositorio
- clona `AoC-Gamers/Steamworks-SDK`
- usa una carpeta SDK como `sdk_158a`
- ejecuta `make deps-linux`
- ejecuta `make build-linux`

Artefacto Linux del workflow:

- `addons/sourcemod/extensions/steamworks.ext.so`
- `addons/sourcemod/scripting/include/steamworks.inc`

Artefacto Windows del workflow:

- `addons/sourcemod/extensions/steamworks.ext.dll`
- `addons/sourcemod/scripting/include/steamworks.inc`

Fuentes de prueba como `scripts/steamwork_test.sp` no se compilan dentro de los
artefactos finales ni se empaquetan en CI.

Limitación conocida
-------------------

La interfaz `ISteamFriends` fue evaluada específicamente para este runtime y
no resultó utilizable desde el contexto del gameserver dedicado de L4D2, aunque
los símbolos existan dentro de las bibliotecas de Steam.

Detalle técnico:

- `docs/ISteamFriends.md`

Documentación técnica adicional
-------------------------------

El directorio `docs/` se usa para documentar interfaces, limitaciones del runtime
y patrones de uso que conviene mantener explícitos.

Documentos actuales:

- `docs/README.md`
- `docs/GetSchemaForGame_550.md`
- `docs/ISteamHTTP.md`
- `docs/ISteamFriends.md`
- `docs/ISteamUserStats.md`
- `docs/TestCoverage.md`

Las rutas pueden sobreescribirse con variables de entorno como `DEPS_DIR`,
`BUILD_DIR`, `HL2SDK_DIR`, `SOURCEMOD_DIR`, `MMSOURCE_DIR`,
`STEAMWORKS_SDK_DIR`, `STEAMWORKS_SDK_NAME` y `VENV_DIR`.

Los scripts tambien cargan opcionalmente un archivo `.env` en la raiz del repo
para parametrizar rutas locales sin editar los scripts.

Archivo de referencia:

- `.env.example`

`STEAMWORKS_SDK_NAME` usa `sdk` por defecto, así que si mantienes múltiples
versiones extraídas del SDK dentro del repositorio puedes cambiar entre ellas
sin editar scripts. Ejemplo:

```bash
make build-linux STEAMWORKS_SDK_NAME=sdk_158a
```
