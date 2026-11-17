# Scripts

| Directorio | Responsabilidad |
|---|---|
| `build/` | Compilar producto Linux/Windows x86 |
| `deps/` | Obtener dependencias explícitas de build |
| `ci/` | Validar el artefacto generado |
| `package/` | Staging, ZIP local y preparación de candidato/reversión |
| `tools/` | Inventario, inspección binaria, perfiles y documentación |

Los ejecutores y ayudantes de pruebas viven en `tests/runners/` y
`tests/helpers/`, fuera de este directorio. Sus nombres, CLI y variables se
conservan; no hay wrappers con rutas antiguas. Cada script resuelve su raíz desde su propio archivo.
Los scripts son backends internos; toda ejecución del proyecto entra por el
Makefile raíz, también en CI. Desde otro cwd usa
`make -f "/ruta/con espacios/SteamWorks/Makefile" <target>`. Una fuente
extraída no requiere `.git` para tests, build o paquetes; el inventario informa
revisión desconocida en ese caso, sin atribuir el Git de un directorio padre.

[Build](../docs/build/README.md), [pruebas](../tests/README.md) y
[política documental](../docs/maintenance/documentation.md) contienen los comandos.
Las herramientas de este directorio no operan servidores. Descargas de
dependencias y build son operaciones diferentes de ejecutar las suites con
dependencias ya preparadas.

La configuración de herramientas vive junto a sus consumidores:
`scripts/build/steamworks-sdk.lock.json` fija los hashes del SDK y
`scripts/package/package-map.json` describe el contenido del paquete. Ambos son
entradas de fuentes/CI; ninguno se distribuye en el ZIP instalable.
