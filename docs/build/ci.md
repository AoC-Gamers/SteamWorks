# CI y validación

El workflow conserva builds Linux/Windows, checks de paquete y suites x86.
Todos los pasos de pruebas, build, empaquetado y validación de producto invocan
targets del Makefile raíz; no se mantienen comandos alternativos en CI. Las claves de caché incluyen las rutas nuevas de dependencias/build/empaquetado.
La validación documental se ejecuta localmente con Python, sin un job de CI
ni una dependencia adicional para los builds. Consulta la
[política documental](../maintenance/documentation.md) para ejecutarla.

El pipeline siguiente describe responsabilidades, no un despliegue automático.
Fuente: Makefile, `.github/workflows/main.yml` y orquestador de pruebas. Los
resultados no ejecutados siguen pendientes y SRCDS requiere un ensayo separado.

```mermaid
flowchart TD
 A[Checkout o fuentes extraidas] --> C[Comparar API con inventario original]
 C --> D[Suites offline requeridas]
 D --> E[Compilar segun toolchain Linux o Windows]
 E --> F[Empaquetar y rechazar material de pruebas]
 F --> G[Guardar resultados fuera de docs]
 G --> H[Revision del cambio]
 H --> I[Publicacion por flujo autorizado]
 H --> J[Ensayo SRCDS separado y autorizado]
```

El orquestador local incluye las pruebas documentales, exige una carpeta nueva
y conserva flags opcionales para
SourceHook/VM y devuelve fallo si una suite requerida falla. Las dependencias
obligatorias no se convierten en skips. El verificador documental no descarga
baselines, opera servidores ni publica; usa solo la biblioteca estándar de Python.
Publicación de artefactos en Actions y publicación de
releases siguen las condiciones existentes del workflow, sin ampliar permisos.

Los paquetes Linux/Windows tienen lista permitida de producto y documentación.
El staging y la validación final rechazan fixtures, SDK, logs y fuentes de tests.
El ZIP de revisión/reversión es una operación local distinta del paquete de
instalación; fuentes y herramientas diagnósticas nunca se mezclan con este último.
Compilar Windows o pasar fixtures locales no acredita ejecución en Windows.

El checkout del SDK está fijado al commit del lock y usa únicamente `sdk_155`.
El job Linux ejecuta los negativos de selección; ambos builds comprueban los
hashes antes de generar producto. El mapa `scripts/package/package-map.json` se
lee desde las fuentes, no desde el ZIP generado, y también forma parte de las
claves de caché. El lock distingue finales de línea de texto de bytes binarios
y entra en las claves de caché de dependencias.

## Notas de canales móviles

`channel/latest` y `channel/develop` conservan sus tags, URLs de release y nombres
estables de ZIP. Tras subir ambos paquetes, CI regenera las notas del canal con
fecha de actualización UTC (ISO 8601), SHA completo y enlace al commit, y enlace
a la ejecución y su intento. Son notas administradas por CI; se reemplazan en
cada actualización completada. La fecha original de publicación de GitHub no
se modifica ni se recrea el release para cambiarla.

Si la carga de archivos falla, no se actualizan las notas con la nueva ejecución.
La publicación continúa sujeta al éxito de los builds y paquetes Linux/Windows.
Las notas conservan GC remoto positivo pendiente y no implican despliegue.
Los releases versionados por tags `v*` mantienen su procedimiento actual.
`make test-release-notes` ejecuta el bloque real del workflow con `gh` y reloj
simulados: no publica, modifica tags ni contacta GitHub.
