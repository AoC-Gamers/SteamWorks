# Análisis con SonarQube Community

`make sonar` ejecuta Cppcheck y después SonarScanner CLI mediante
`scripts/tools/make_sonar.py`. Sigue la separación de configuración y scripts
usada por BanSystem, adaptada al plugin comunitario
[sonar-cxx](https://github.com/SonarOpenCommunity/sonar-cxx).
No usa SonarCFamily ni su Build Wrapper. Requiere Cppcheck y SonarScanner en PATH,
un runtime Java compatible con el scanner/plugin y CXX instalado en SonarQube.
Las herramientas se instalan explícitamente; este target no las descarga.

## Configuración local

Copia `.env.example` a `.env` y `env.secret.example` a `env.secret`.
Configura `SONAR_HOST_URL` con la URL de tu instancia y guarda `SONAR_TOKEN`
únicamente en `env.secret`, con permisos 600 en Linux, o en el entorno del proceso.
Ambos archivos reales están ignorados por Git. Los ejemplos no contienen tokens
ni direcciones de instalaciones concretas.

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
de compilación. `sonar-project.properties` limita las fuentes a `extension/` y
`scripts/`; no indexa SDK, dependencias descargadas, tests ni resultados. SourcePawn
no se presenta como C++. Los sufijos CXX se configuran para este proyecto, sin
cambiar globalmente los demás proyectos del servidor.

Cada ejecución guarda base filtrada, XML y trabajo del scanner bajo un directorio
nuevo de `.build/sonar/`. Un fallo de Cppcheck o un XML inválido impiden ejecutar
el scanner; sus fallos y los del gate se propagan a Make. Los resultados generados,
configuración local y secretos no se empaquetan como producto.

## Pruebas y límites

```bash
make sonar-tooling-test
```

La suite usa ejecutables controlados, sin red ni credenciales reales. Comprueba
Make desde otra ruta con espacios, prioridad de variables, separación del token,
alcance y flags x86, XML y propagación de errores. Se incluye en la integración
y CI como prueba offline; CI no ejecuta un análisis remoto de SonarQube.
No acredita un primer escaneo real ni compatibilidad del analizador con todos los
headers del SDK. Sonar complementa ASan/UBSan y los ensayos SRCDS; no los sustituye.
La recepción real de `OnUserStatsUnloaded` conserva su
[límite funcional](../stats/README.md#descarga-de-caché-en-l4d2-dedicado-x86).

Referencias: [base de compilación de CXX](https://github.com/SonarOpenCommunity/sonar-cxx/wiki/sonar.cxx.jsonCompilationDatabase),
[importación Cppcheck](https://github.com/SonarOpenCommunity/sonar-cxx/wiki/sonar.cxx.cppcheck.reportPaths)
y [SonarScanner CLI](https://docs.sonarsource.com/sonarqube-server/analyzing-source-code/scanners/sonarscanner).
