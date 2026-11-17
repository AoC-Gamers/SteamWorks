# Pruebas

La integración compara la API con un baseline independiente y ejecuta suites
offline sin administrar servidores. Elige una carpeta **nueva** de salida:

```bash
make test-typed-boundary
make test-resource-lifecycle
make test TEST_OUTPUT=.build/test-results/mi-ejecucion
```

Makefile es la interfaz obligatoria para ejecutar pruebas tanto localmente como
en CI; los scripts de `runners/` son backends internos. `make test-<suite>` usa
los mismos ejecutores que la integración. Para otro cwd usa
`make -f "/ruta/con espacios/SteamWorks/Makefile" <target>`; se admiten fuentes
sin `.git`. Las salidas relativas explícitas se interpretan desde el cwd del usuario.
Se requiere Python 3, C++14, multilib x86/UBSan y SDK 1.55. La suite binaria
requiere utilidades ELF/PE; no carga un juego. Con flags sin `-m32` su ejecución
de instrucciones x86 se omite explícitamente; la integración predeterminada usa
`-m32` y debe ejecutarla. La falta de una dependencia obligatoria es un error.

## Estructura

`tests/` reúne las fuentes y las herramientas de prueba:

| Directorio | Contenido |
|---|---|
| `runners/` | Ejecutores de suites y orquestador de integración |
| `helpers/` | Generadores y compilación de proveedores/VM diagnóstica |
| `native/`, `integration/`, `python/` | Fuentes de regresión y contratos offline |
| `support/`, `fixtures/` | Dobles compartidos y entradas protegidas |
| `runtime/`, `diagnostics/` | Harnesses SRCDS explícitos y probes de investigación |

## Catálogo

Los targets `make test-<suite>` delegan en `tests/runners/test-<suite>.sh`.
`make test-ci` ejecuta selección SDK, paquetes, interfaz Make y suites nativas,
incluida SourceHook y las pruebas offline de notas de canales, sin reincorporar
la prueba documental a CI.
`make test-documentation` y `make check-docs` son comprobaciones locales.
`make sonar-tooling-test` prueba la orquestación de Cppcheck/SonarScanner con dobles,
sin red ni tokens reales; forma parte de la integración local. Sus pruebas y las
del tooling de cobertura quedan fuera del pipeline y de `make test-ci`.
La integración también ejecuta `make_interface_test.py`, que verifica rutas con
espacios, opciones, plataforma y propagación de errores de los backends.

| Suite | Propósito | Dependencia real/simulada | No demuestra |
|---|---|---|---|
| extension-entrypoint | Un propietario por DSO, doble nombre, reentrada, fallos y recarga | Factory/wrappers de producción + cargador controlado, ASan/UBSan x86 y negativo | Nueva aceptación SRCDS |
| stats-callbacks | CallResults por solicitud, caché, aislamiento cliente/GameServer, errores y teardown reentrante | SDK real + entrega simulada por contexto, ASan/UBSan x86 y negativo sin filtro de contexto | Recepción Steam real |
| ticket-ownership | Inicialización y propiedad no copiable del lector heredado; ticket sintético y todos sus prefijos truncados | Headers de producción y ASan/UBSan x86; negativos de inicialización/copia | Formato de tickets Steam actuales ni uso en autenticación |
| stats-natives | Salidas intactas al fallo, AppID separado de identidad e índices independientes de entidad | natives + host Steam simulado | Disponibilidad real de stats |
| detours | Creación/habilitación y rollback, backup optimizado | CDetour + host simulado | Módulos reales del motor |
| init-coexistence | Observador init y cadena de hooks ajenos | Bytes y callsites x86 + dobles | Convivencia de toda la pila de plugins |
| public-ip | IPv4 y char signed/unsigned | Proveedor simulado | Dirección pública real |
| steamid-string | Capacidades, truncamiento, NUL legacy e índices en identidad/licencias/grupos | Conversión SourcePawn simulada | Autorización real |
| typed-boundary | Registro, tipos, buffers HTTP/GC y negativos de compilación | SDK real + proveedores simulados | ABI completa del runtime |
| resource-lifecycle | Callbacks, hooks, requests y teardown; negativo GC | Producción + hosts simulados, optimización auth | Parada real SRCDS |
| restart-forward-semantics | Resultados de reinicio y reentrada | Forward simulado | Agregación completa de VM |
| binary-evidence | Hashes/perfiles y relocalización de prefijos | Binarios SDK reales, ejecución x86, Python | Llamada init real ni runtime Windows |
| debug-log | Máscara, I/O y rotación | Filesystem temporal + backend simulado | Autenticación válida |
| http-stream-context | Dos celdas, wire independiente y negativo original | HTTP simulado con layout independiente | Entrega de un servidor remoto |
| auth-forward | Marshalling y agregación real de forwards | VM SourcePawn real + Steam simulado; opcional | Ticket auténtico |
| auth-binding | Objetivo motor 014, otro objeto SDK 014 y doble 015 rechazado | SourceHook real + dos proveedores simulados; opcional | Login Steam real |
| sourcepawn-environment | Destrucción VM y ciclos de propiedad | VM instrumentada real, ASan; opcional | VM de producción ni estabilidad SRCDS |

La suite `auth-binding` también ejecuta las macros reales de SourceHook que
preparan punteros a miembro: fija su representación GNU x86 de dos palabras y
comprueba límites/canarios con ASan/UBSan. Un acceso a la segunda palabra no es
por sí solo un desbordamiento de un objeto de ocho bytes en esa ABI; no se
transfiere este resultado a otro compilador o arquitectura.

`client_index_test.cpp` se compila en las suites de stats y SteamID para probar
los seis natives de producción que reciben un índice de cliente. Los dobles
mantienen independientes el slot conectado y la entidad; cubren ambos extremos
del rango, ausencia de entidad, referencias de entidad e índices inválidos,
clientes ausentes/desconectados y ausencia de trabajo o escrituras tras el rechazo.

Además, `tests/python/sdk_selection_test.py` verifica el release, los 47 hashes
y los rechazos de headers/bibliotecas cambiados o ausentes; la integración realiza
preflight del SDK antes de ejecutar suites. `tests/python/review_artifacts_test.py` comprueba paquetes/reversión;
`runtime_evidence_test.py` valida perfiles e identidad; `documentation_test.py`
comprueba estructura, enlaces y rechazos de contenido. Los ejecuta la integración.
Los negativos HTTP estricto y GC se generan con mutaciones mínimas de producción:
no requieren Internet, Git ni informes retirados y deben fallar con el mensaje
esperado. Un error de compilación o de sanitizer no cuenta como detección válida.

## Variables y suites opcionales

| Variable | Uso |
|---|---|
| `TEST_OUTPUT`, `TEST_OPTIONS` | Carpeta nueva y selección de suites de `make test` |
| `LINUX_DEPS_DIR`, `AMBUILD_PYTHON`, `HL2SDK_ROOT` | Dependencias y Python de AMBuild para los targets Make |
| `STEAMWORKS_SDK_DIR` | Headers y biblioteca de referencia del SDK |
| `CXX`, `TEST_CXXFLAGS`, `TEST_BUILD_DIR` | Compilador, flags y salida de cada suite nativa |
| `AMTL_DIR`, `SOURCEMOD_DIR`, `MMSOURCE_DIR` | Headers AMTL, VM y SourceHook reales |
| `SPCOMP`, `SOURCEPAWN_VM` | Compilador Pawn y biblioteca VM de arquitectura coincidente |
| `VM_TEST_CXX`, `VM_TEST_FLAGS`, `VM_TEST_BUILD_DIR` | Toolchain y salida de VM, separados de los tests nativos |

`TEST_OPTIONS="--with-auth-forward --with-auth-binding --with-sourcepawn-environment"`
selecciona las suites opcionales en `make test`. Esos flags
exigen sus dependencias al seleccionarse; no convierten una ausencia en éxito.
La última requiere VM y runtime ASan compatibles sin supresiones. Consulta
[integración offline](integration/README.md).

`results.json` y logs van a la salida elegida, nunca a `docs/`. El resultado de
API excluye números de línea y metadatos, no campos contractuales. El inventario
original de API (`fixtures/api/api-inventory-p0.json`) no se regenera a partir del estado actual para hacer pasar una comparación.

## Alcance y operación

`make test-coverage TEST_OUTPUT=<nueva-carpeta>` mide líneas del código nativo
mediante GCC/gcov x86, manteniendo sanitizadores y controles negativos. Requiere
la base de compilación del build actual en `LINUX_BUILD_DIR`. Solo los ejecutables
positivos aportan hits; el producto compilado sin ejecutar aporta ceros. Genera
un informe genérico para Sonar y un recibo ligado a las fuentes, sin atribuir
cobertura de ramas ni aceptación real a los dobles locales. Consulta
[configuración e importación](../docs/build/sonar.md#cobertura-real-x86).
`make coverage-tooling-test` verifica la contabilidad y sus controles offline.

`make test-cxx-parser CXX_PARSER_CLASSPATH=<bibliotecas-del-plugin-y-API>` es
una regresión offline explícita con el parser CXX instalado en modo estricto,
cuatro negativos y comparación ABI de los exports GNU x86. Requiere JDK 17 o
posterior y multilib; no descarga herramientas. Su aislamiento de gramática no
acredita el contexto completo de includes, que se valida con `make sonar`.

`make deps-test-coverage` instala explícitamente coverage.py fijado en un venv
aislado. `make test-python-coverage TEST_OUTPUT=<nueva-carpeta>` ejecuta todas
las suites Python con medición del producto canónico y sus subprocesos. Mantiene
líneas no ejecutadas, excluye copias temporales y genera XML y recibo para Sonar.
Python usa la arquitectura del host, separada de la cobertura nativa x86.
Los informes de ambos lenguajes deben corresponder a las mismas fuentes actuales;
consulta [cobertura Python](../docs/build/sonar.md#cobertura-real-python).

- [Cobertura y evidencia histórica](COVERAGE.md)
- [Fixtures y procedencia](fixtures/README.md)
- [Harnesses SRCDS de ejecución explícita](runtime/README.md)
- [Probes diagnósticos](diagnostics/README.md)
- [Pipeline y separación de publicación](../docs/build/ci.md)

Los tests locales validan los contratos bajo proveedores controlados. En GC
se comprueban ABI, buffers, forwards y ciclo de vida; la API se conserva por
compatibilidad y su utilidad depende del soporte del juego y del contexto Steam.
No se acepta una revisión nueva por compartir nombre con un binario anterior.

`make test-sdk-incremental TEST_OUTPUT=<nueva-carpeta>` delega en
`runners/test-sdk-incremental.py` y requiere además AMBuild y los
árboles reales SourceMod/Metamod/HL2SDK. Configura y compila el producto x86,
cambia solo copias privadas de un header, biblioteca no usada por Linux,
header ausente y header adicional, y exige rechazo antes de reescribir el ELF.
Restaura cada entrada y comprueba recuperación del build. CI Linux la ejecuta
tras el build de producto; no inicia servidores ni modifica el SDK original.

`make test-release-notes` comprueba creación y actualización de los canales
Latest/Develop como borradores, sus notas UTC/commit/run y los fallos de
preparación, eliminación y carga con CLI GitHub
simulado. No publica ni modifica releases; también forma parte de la integración.

`make test-public-api` conserva íntegro el inventario original y exige las dos
adiciones de stats declaradas manualmente en `helpers/api_contract.py`. Las
pruebas negativas rechazan adiciones extra, duplicadas o con otra firma/registro,
y cualquier alteración del contrato original; no se regenera el baseline.
