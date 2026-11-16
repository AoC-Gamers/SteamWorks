# Scripts

| Directorio | Responsabilidad |
|---|---|
| `build/` | Compilar producto Linux/Windows x86 |
| `deps/` | Obtener dependencias explícitas de build |
| `ci/` | Validar el artefacto generado |
| `package/` | Staging, ZIP local y preparación de candidato/reversión |
| `tools/` | Inventario, inspección binaria, perfiles y documentación |
| `tests/` | Ejecutores offline; `helpers/` compila/genera entradas diagnósticas |

Se conservan los 32 nombres de ejecutores anteriores, con sus CLI y variables;
no hay wrappers en la raíz. Cada script resuelve su raíz desde su propio archivo.
Desde otro cwd usa su ruta absoluta y entrecomilla rutas con espacios. Una fuente
extraída no requiere `.git` para tests, build o paquetes; el inventario informa
revisión desconocida en ese caso, sin atribuir el Git de un directorio padre.

[Build](../docs/build/README.md), [pruebas](../tests/README.md) y
[política documental](../docs/maintenance/documentation.md) contienen los comandos.
Los scripts offline no operan servidores. Un helper para compilar una extensión
de fixture no la instala ni autoriza hacerlo. Descargas de dependencias y build
son operaciones diferentes de ejecutar las suites con dependencias ya preparadas.
