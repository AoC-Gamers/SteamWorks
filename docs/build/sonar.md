# Análisis con SonarQube Community

`make sonar` ejecuta Cppcheck y después SonarScanner CLI mediante
`scripts/tools/make_sonar.py`. Sigue la separación de configuración y scripts
usada por BanSystem, adaptada al plugin comunitario
[sonar-cxx](https://github.com/SonarOpenCommunity/sonar-cxx).
No usa SonarCFamily ni su Build Wrapper. Requiere Cppcheck y SonarScanner en PATH,
un runtime Java compatible con el scanner/plugin y CXX instalado en SonarQube.
Las herramientas se instalan explícitamente; este target no las descarga.
`SONAR_CPPCHECK` y `SONAR_SCANNER` permiten seleccionar los ejecutables por nombre
en PATH o mediante rutas absolutas/relativas a las fuentes.

## Configuración local

Copia `.env.example` a `.env` y configura `SONAR_HOST_URL` con la URL de tu
instancia. Para una instalación nueva, copia `.env.secret.example` a
`.env.secret` y guarda allí `SONAR_TOKEN`, con permisos 600 en Linux, o
inyéctalo en el entorno del proceso. Los archivos reales están ignorados por Git.
Los ejemplos no contienen tokens ni direcciones de instalaciones concretas.

Si aún guardas el token en `env.secret`, renombra ese archivo a `.env.secret`
sin sobrescribir un destino existente. Sonar solo lee el nombre nuevo.

`SONAR_PROJECT_KEY` y `SONAR_PROJECT_NAME` permiten cambiar la identidad del
proyecto. `SONAR_QUALITY_GATE_WAIT=true` espera el resultado del quality gate;
`SONAR_QUALITY_GATE_TIMEOUT` expresa el límite en segundos. Si se desactiva esa
espera, la finalización del scanner no acredita que el gate haya pasado.
Las variables exportadas tienen prioridad sobre los archivos. El script lee los
valores sin evaluar shell y rechaza tokens dentro de `.env`.
El token solo se transmite al scanner por entorno; no se escribe en propiedades,
argumentos ni informes. La salida del scanner se sanea antes de mostrarla.
Se exige HTTPS, excepto para ensayos por loopback.

## Build x86 y ejecución

Primero genera un build actual con las dependencias y el SDK 1.55:

```bash
make build-exts-linux
make sonar-cppcheck
make sonar
```

El build Linux genera `compile_commands.json` dentro de `LINUX_BUILD_DIR` mediante
AMBuild. Esa base conserva los flags `-m32`, macros e includes reales. Si usas otro
directorio, conserva el mismo `LINUX_BUILD_DIR` en las tres invocaciones; también
puedes indicar `SONAR_COMPILE_COMMANDS` en `.env` (ruta relativa a las fuentes o
absoluta). Regenera el build después de modificar fuentes, macros o dependencias;
la base de compilación describe la configuración, no sustituye las pruebas.

`make sonar-cppcheck` es exclusivamente local y no necesita endpoint ni token.
`make sonar` repite Cppcheck para generar un informe nuevo y **envía el análisis
al SonarQube configurado**. No se ejecuta automáticamente al compilar o probar.
Los hallazgos de Cppcheck se importan al servidor; su evaluación depende de las
reglas activas en el perfil CXX y del quality gate del proyecto. Instalar el plugin
no activa por sí solo todas las reglas de analizadores externos.

El script exige las unidades C/C++ de `extension/` y rechaza órdenes sin `-m32`.
Filtra las unidades de terceros de la base, conservando sus includes como entradas
de compilación. Consulta las macros de identidad/versión y plataforma del GCC/Clang real con
los flags reales de cada orden, incluidos `-m32`, estándar, SSE, PIC,
optimización y excepciones: esas macros predefinidas no aparecen en la base
de AMBuild y AMTL las necesita. No se inventan versiones ni se suprimen `#error`.
La consulta conserva el directorio de compilación y resuelve los includes
relativos allí, sin crear objetos ni archivos de dependencias.
También captura los includes implícitos del compilador. Para CXX genera una base
separada con el contexto C++ común `__global__`: los headers indexados no tienen
su propia orden de compilación. Las órdenes de cada unidad conservan prioridad;
la base usada por Cppcheck no contiene esa entrada específica de CXX.
El script pasa al scanner la configuración de `.env` y del entorno, junto con
las reglas fijas del proyecto. El análisis usa UTF-8 y cubre `extension/` y
`scripts/`, con las cachés de Python excluidas. Los sufijos CXX se limitan a C/C++
y la recuperación de errores sintácticos queda desactivada para este proyecto.

Cada ejecución guarda base filtrada, XML y trabajo del scanner bajo un directorio
nuevo de `.build/sonar/`. Un fallo de Cppcheck o un XML inválido impiden ejecutar
el scanner; sus fallos y los del gate se propagan a Make. Los resultados generados,
configuración local y secretos no se empaquetan como producto.
Los fallos de ejecución de Cppcheck conservan un diagnóstico saneado junto a la
base de esa ejecución para distinguir instalación, configuración y hallazgos.
Los errores de preprocesado, sintaxis o internos del analizador impiden enviar
un informe parcial a SonarQube, aunque Cppcheck devuelva código cero.

## Cobertura real x86

Con GCC, gcov de la misma versión y las dependencias del build disponibles:

```bash
make test-coverage TEST_OUTPUT=.build/coverage/run-001
```

Usa un directorio nuevo y el mismo `LINUX_BUILD_DIR` del build. El target ejecuta
la integración con `-m32 --coverage -fprofile-abs-path`, incluidos SourceHook real,
ASan/UBSan y los controles negativos. Solo incorpora contadores de los ejecutables
positivos originales; los binarios mutados no aportan cobertura. Compila también
las unidades reales del producto sin ejecutarlas para incorporar sus líneas
ejecutables con cero hits, según GCC. No infiere cobertura a partir del número
de comprobaciones ni de la mera compilación.

El resultado contiene `coverage.xml` en formato genérico de Sonar, contadores y
JSON de gcov, logs y `receipt.json`. La métrica es cobertura de **líneas** de
`extension/`, combinada entre las configuraciones probadas; no mide ramas.
No incluye dobles, tests ni dependencias como producto. Estas ejecuciones locales
con proveedores controlados no acreditan recepción real de callbacks en SRCDS.
La cobertura Python se mide por separado, como se describe a continuación.

Configura en `.env` el recibo de esa ejecución y analiza:

```dotenv
SONAR_COVERAGE_RECEIPT=.build/coverage/run-001/receipt.json
```

```bash
make sonar
```

El script verifica las huellas de fuentes, pruebas e informe antes del análisis
y alrededor de la ejecución del scanner. Rechaza informes modificados, fuentes
distintas o suites fallidas. Tras cambios en esas entradas, repite la medición
en otro directorio. Sin `SONAR_COVERAGE_RECEIPT` no se importa cobertura.
El quality gate conserva sus umbrales: importar cobertura no garantiza aprobarlo.

## Cobertura real Python

Instala explícitamente coverage.py 7.13.5 en un entorno virtual aislado y ejecuta
las suites Python existentes, sin iniciar servidores ni contactar Sonar:

```bash
make deps-test-coverage
make test-python-coverage TEST_OUTPUT=.build/coverage/python-001
```

El target usa `COVERAGE_VENV_DIR` (`.deps/test-coverage` por defecto) y
`COVERAGE_PYTHON`. No instala dependencias al probar. La instalación requiere
Python con venv y acceso a PyPI; la versión está fijada en
`tests/requirements-coverage.txt`. Python se ejecuta en la arquitectura del host;
esta medición es independiente de los contadores GCC del producto x86.

Coverage.py mide las pruebas y sus subprocesos con `patch = subprocess`, combina
los datos y conserva `.coverage`, datos por proceso, JSON, log, XML genérico y
recibo bajo la salida nueva. El informe incluye **todas** las fuentes Python
canónicas de `scripts/`, también las líneas no ejecutadas. No incluye los tests,
dependencias ni copias temporales; no remapea mutantes al producto. Los casos de
orquestación también llaman al módulo de producción importado para comprobar
los caminos de éxito, rechazo y propagación de fallos sin depender de copias CLI.
La medición es de líneas, sin atribuir cobertura de ramas ni aceptación SRCDS.

Configura ambos recibos en `.env` después de medir las mismas fuentes:

```dotenv
SONAR_COVERAGE_RECEIPT=.build/coverage/run-001/receipt.json
SONAR_PYTHON_COVERAGE_RECEIPT=.build/coverage/python-001/receipt.json
```

`make sonar` valida ambos recibos antes y después del scanner e importa sus XML
mediante `sonar.coverageReportPaths`. Un recibo de otro tipo, alterado, fallido o
desactualizado bloquea el análisis; no se reutiliza silenciosamente ni se omite
su métrica. Si cambia una fuente, prueba o entrada Make, regenera ambos informes.
No se cambian exclusiones de Sonar, baseline de código nuevo ni umbrales del gate.

## Pruebas y límites

```bash
make sonar-tooling-test
make coverage-tooling-test
```

La suite usa ejecutables controlados, sin red ni credenciales reales. Comprueba
Make desde otra ruta con espacios, prioridad de variables, separación del token,
alcance y flags x86, XML y propagación de errores. Las pruebas de Sonar y cobertura
permanecen en la integración local y en sus targets explícitos; no forman parte
del pipeline de CI ni de `make test-ci`. CI tampoco ejecuta análisis remotos de SonarQube.
No acredita un primer escaneo real ni compatibilidad del analizador con todos los
headers del SDK. Sonar complementa ASan/UBSan y los ensayos SRCDS; no los sustituye.
Se corrigieron dos límites de la gramática CXX 2.3.1 con formas equivalentes:
el argumento predeterminado de `DebugLog` agrupa su producto entre paréntesis y
los exports GNU usan `[[gnu::visibility("default")]]`, manteniendo enlace C y
visibilidad explícita. Windows conserva `__declspec(dllexport)`. No se redefinen
atributos como macros vacías ni se excluyen fuentes. `sonar.cxx.errorRecoveryEnabled=false`
exige análisis estricto: un error de parser interrumpe el análisis en lugar de
publicar métricas parciales. La suite de
cobertura comprueba contadores GCC x86 reales, exclusión de mutantes y rechazo
de informes alterados; la de Sonar verifica su importación y rechazo anticipado.

`make test-cxx-parser` requiere un JDK 17 o posterior y `CXX_PARSER_CLASSPATH`
con las bibliotecas del mismo plugin CXX instalado y la API del scanner. No las
descarga ni inicia Sonar. Aísla las declaraciones de producción para probar la
gramática estricta y rechaza cuatro controles negativos, incluidas las formas
anteriores. Compara además ambos exports con GCC x86 bajo `-fvisibility=hidden`:
deben conservar símbolos de función C `GLOBAL DEFAULT`. Este ensayo acotado no
sustituye el análisis completo de includes mediante `make sonar`, ni la
compilación del producto y los sanitizadores. La suite de logging verifica
también la rotación predeterminada a un MiB con escrituras reales.
La recepción real de `OnUserStatsUnloaded` conserva su
[límite funcional](../stats/README.md#descarga-de-caché-en-l4d2-dedicado-x86).

Referencias: [subprocesos coverage.py](https://coverage.readthedocs.io/en/7.13.5/subprocess.html),
[base de compilación de CXX](https://github.com/SonarOpenCommunity/sonar-cxx/wiki/sonar.cxx.jsonCompilationDatabase),
[importación Cppcheck](https://github.com/SonarOpenCommunity/sonar-cxx/wiki/sonar.cxx.cppcheck.reportPaths)
y [SonarScanner CLI](https://docs.sonarsource.com/sonarqube-server/analyzing-source-code/scanners/sonarscanner).
