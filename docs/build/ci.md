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
