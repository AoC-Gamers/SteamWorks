# Dependencias

| Dependencia | Finalidad |
|---|---|
| Python y AMBuild | Configurar y ejecutar el build |
| C++ x86 y multilib en Linux | Generar ELF32 para L4D2 |
| Visual C++ x86 y PowerShell en Windows | Generar DLL de Windows |
| SourceMod 1.12, Metamod:Source 1.12, HL2SDK L4D2 y AMTL | Headers y soporte del host |
| Steamworks SDK 1.55 | Declaraciones y enlace, no runtime de servidor |
| spcomp y SourcePawn VM diagnóstica | Suites opcionales offline, fuera del producto |

Los scripts `scripts/deps/fetch-linux-deps.sh` y `fetch-windows-deps.ps1` resuelven
las dependencias de build según sus variables y URLs explícitas. Antes de cambiar
una revisión, revisa las declaraciones y la [frontera tipada](../architecture/typed-boundary.md).
Las constantes de versiones de interfaz se comprueban al compilar.

## Obtener Steamworks SDK 1.55

La fuente oficial es el [portal de Steamworks SDK](https://partner.steamgames.com/doc/sdk).
Accede con una cuenta autorizada y bajo las condiciones de Valve. Este proyecto
requiere exclusivamente **Steamworks SDK 1.55**, identificado como `sdk_155`;
no admite 1.55a ni sustituye automáticamente el SDK por una versión más reciente.
El enlace principal del portal corresponde a la última versión disponible y
puede no ser 1.55: utiliza una copia autorizada de la versión exacta, no renombres
un SDK distinto para intentar validarlo.

Extrae el SDK y señala el directorio que contiene `public/steam` y
`redistributable_bin` mediante `STEAMWORKS_SDK_DIR`. Por ejemplo, desde la raíz:

```bash
make validate-sdk STEAMWORKS_SDK_DIR=/ruta/al/sdk_155
make build-exts-linux STEAMWORKS_SDK_DIR=/ruta/al/sdk_155
```

El lock de fuentes `scripts/build/steamworks-sdk.lock.json` comprueba los
47 archivos requeridos; el nombre de la carpeta por sí solo no acredita versión.
El SDK permanece fuera del paquete instalable y de las fuentes públicas del proyecto.

El workflow del mantenedor obtiene su copia fijada desde el repositorio privado
`AoC-Gamers/Steamworks-SDK`. Ese repositorio sirve a la automatización con acceso
autorizado; no es un enlace público de descarga. Para compilar localmente puedes
aportar tu propia copia del SDK 1.55 por la variable indicada, sin acceso a ese
repositorio. Las condiciones aplicables siguen siendo las de Valve.

Las pruebas usan `STEAMWORKS_SDK_DIR`; no descargan un baseline de API ni consultan
Internet para obtener perfiles. Sus fixtures están incluidos en las fuentes.
El build de la VM diagnóstica aplica los parches de `patches/` a un checkout de
SourcePawn seleccionado: su toolchain, ASan y arquitectura deben coincidir con
los del ejecutor. No sustituye la VM de SourceMod en un servidor.

El lock de fuentes verifica 47 entradas del SDK 1.55. Las versiones distintas
y los headers/bibliotecas modificados se rechazan; las rutas externas con espacios
se admiten si su contenido coincide. Los SDK de comparación históricos en fixtures
son metadatos, no dependencias operativas.

Los headers se comparan con normalización CRLF→LF, coincidente con los blobs del
commit fijado del SDK. Las bibliotecas se comparan por bytes exactos; ningún otro
cambio de contenido se normaliza o permite.
