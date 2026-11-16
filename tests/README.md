# Pruebas nativas deterministas

## Integración local y artefactos

P5-R2 ejecuta diez suites con `--with-auth-forward --with-sourcepawn-environment`.
`VM_TEST_CXX` y `VM_TEST_FLAGS` seleccionan el compilador/runtime instrumentado
para esas dos suites, conservando el toolchain de las nativas. El preparador de
artefactos admite `--label p5-r2`; once pruebas verifican su contrato y la
restauración del candidato real desde el ZIP de reversión.

`bash scripts/test-sourcepawn-vm-asan.sh` construye una VM de prueba desde fuentes
fijadas con un parche de ownership, sin cambiar las cabeceras públicas ni tocar
bibliotecas instaladas. Ejecuta 16 ciclos de destrucción y la regresión de
autenticación con ASan/LSan activos. Requiere Clang con su runtime ASan, AMBuild,
`SOURCEMOD_DIR`, `SPCOMP` y una salida nueva en `VM_TEST_BUILD_DIR`. Ver
[diagnóstico y reproducción](../docs/runtime-compatibility/sourcepawn-vm-asan.md).

`bash scripts/test-auth-forward.sh` requiere `SOURCEMOD_DIR`, `SPCOMP` y
`SOURCEPAWN_VM`: ejercita el hook de producción con CForward real y un receptor
SourcePawn compilado en una VM aislada. No arranca SRCDS ni llama a Steam.
`test-integration.py --with-auth-forward` añade esta prueba a las ocho suites
habituales. El script falla si faltan sus dependencias explícitas. Ver
[T36](../docs/runtime-compatibility/t36-auth-forward.md) para resultados y límites.

`python3 scripts/test-integration.py --output .build/validacion-nueva` compara
el contrato público con P0 y ejecuta las siete suites siguientes más las nueve
pruebas de empaquetado/reversión. Requiere AMTL para SteamID y admite las variables
de toolchain descritas abajo. La salida debe ser nueva para conservar evidencia.
Se registran tiempo y código de salida por suite, sin arrancar servidores.

`python3 tests/review_artifacts_test.py` valida el contrato de archivos del
preparador local, con ELF de fixture, no la ABI. La ejecución sobre la captura
real de reversión está documentada en [P5](../docs/runtime-compatibility/p5-integration.md).

## Stats

Desde la raíz del repositorio:

```bash
bash scripts/test-stats-natives.sh
```

Requiere Bash, Steamworks SDK 1.58a en `sdk_158a/` (o la ruta indicada por
`STEAMWORKS_SDK_DIR`) y un compilador C++14 con UBSan (`g++` por defecto). Se puede
seleccionar otro compilador mediante `CXX`, cambiar la salida con
`TEST_BUILD_DIR` o añadir flags mediante `TEST_CXXFLAGS`.
Ejemplo con un toolchain multilib disponible:

```bash
TEST_CXXFLAGS=-m32 TEST_BUILD_DIR="$PWD/.build/tests-stats-x86" bash scripts/test-stats-natives.sh
```

Las dos suites incluyen el archivo C++ de producción y llaman los natives a través
de su tabla de registro real. Los guards de headers del test sustituyen las
dependencias Steam/SourceMod solo en esa unidad de compilación. La extensión no
contiene un modo especial de pruebas. Se usa el `CSteamID` real del SDK para
observar sus campos y representación de 64 bits; los servicios de SourceMod,
Utils y Stats se simulan. No se enlaza ni inicia un runtime Steam.

Los 32 escenarios cubren fallo sin escritura, fallo después de escribir en el
temporal, éxito con valor negativo y con cero, dirección inválida de nombre o
salida, interfaz ausente y cliente inexistente/desconectado. Se comprueban el
retorno, un centinela de salida, canarios adyacentes, el overload entero/flotante,
el número de consultas y los errores de conversión. La precedencia existente se
conserva: interfaz ausente devuelve fallo antes de resolver direcciones; las
variantes cliente validan al jugador antes de convertir argumentos.

El ejecutable acepta opcionalmente el nombre de un native para aislar una
variante, por ejemplo:

```bash
.build/tests-stats/stats_natives_test SteamWorks_GetStatFloat
```

`stats_requests_test` añade 34 escenarios sobre `RequestStats` y
`RequestStatsAuthID`: AppID del contexto, distinto, cero, contexto desconocido,
interfaces ausentes, fallo de solicitud asíncrona, AccountID con bit alto,
cliente inválido/desconectado/bot/no autorizado, identidad de 64 bits inconsistente,
tipo/universo/instancia inválidos y fallback desde AccountID validado. Compara la
identidad exacta entregada al doble de Steam, comprueba que no se consulta con
AppID ajeno y que las solicitudes no usan el constructor legacy basado en `params`.
Las variantes de lectura conservan el doble legacy porque su identidad está
fuera del alcance de estas pruebas de salida.

```bash
.build/tests-stats/stats_requests_test SteamWorks_RequestStatsAuthID
```

Estas pruebas unitarias no cargan la extensión en un servidor ni sustituyen las
pruebas de stats reales y marshalling SourceMod. Ver el
[informe de salidas](../docs/runtime-compatibility/p1-a-stats.md) y el
[informe de AppID e identidad](../docs/runtime-compatibility/p1-b-stats-identity.md).

## Propietario y callbacks de detours

```bash
bash scripts/test-detours.sh
```

Usa las mismas opciones `CXX`, `TEST_BUILD_DIR`, `TEST_CXXFLAGS` y
`STEAMWORKS_SDK_DIR`. Incluye el código de producción de `swgsdetours` y simula
el host, símbolos, factoría CDetour, parcheo y trampolines. No ejecuta instrucciones
del motor ni prueba la ABI de la entrada legacy.

Los 26 casos comprueban precondiciones, creación completa antes de habilitar,
fallo de cada creación, limpieza parcial, precedencia de gamedata/exports,
diagnóstico único, cierre repetido, creación posterior a fallo, argumentos y
retorno del callback original, ausencia de hooks tras fallo y orden del wrapper
de shutdown. Se comprueba que el propietario no pueda copiar sus recursos.

Ver [alcance, limitaciones y evidencia](../docs/runtime-compatibility/p1-c-detours.md).

## IP pública IPv4

```bash
bash scripts/test-public-ip.sh
```

Usa las mismas opciones de compilador, salida, flags y SDK. Compila y ejecuta
30 escenarios con `-fsigned-char` y otra vez con `-funsigned-char`, siempre con
UBSan. Los dos entrypoints y su tabla son de producción; `SteamIPAddress_t` y
sus constructores son del SDK real. SourceMod y el servicio GameServer son dobles.

Comprueba orden de octetos, IPv4 no establecida, IPv6 establecida/no establecida,
interfaz ausente, direcciones con bit alto y ceros interiores, conservación del
patrón de bits de la celda, canarios de salida y errores al convertir la dirección
del destino. No valida el transporte de `SteamIPAddress_t` desde una vtable real.

El ejecutable acepta un caso y opcionalmente un native para aislar una prueba:

```bash
.build/tests-public-ip/public_ip_test_signed "IPv6 nonzero prefix rejected" SteamWorks_GetPublicIPCell
```

Ver [contrato y resultados](../docs/runtime-compatibility/p1-d-public-ip.md).

## Conteo y buffers de SteamID textual

```bash
AMTL_DIR=/ruta/a/sourcemod/public/amtl bash scripts/test-steamid-string.sh
```

Requiere los headers de AMTL usados por SourceMod, además del SDK. El runner usa
`AMTL_DIR` si se indica; por defecto busca `public/amtl` bajo `SOURCEMOD_DIR` o
`.deps/exts-linux/sourcemod-1.12/`. No descarga dependencias ni copia el formateador.

Los 19 casos ejecutan el native registrado de producción con `CSteamID` real y
`ke::SafeVsprintf` de AMTL, la implementación a la que delega el `Format` de
SourceMod. El host/memoria/clientes se simulan. Comprueban conteo incluyendo NUL,
texto y terminación exactos, truncamiento, capacidad mínima/cero/negativa,
dirección inválida, cliente inválido/desconectado y precedencia de errores.
El límite `uint64` es un caso de formato, no una identidad Steam aceptada en red.

Se puede aislar un caso:

```bash
.build/tests-steamid-string/steamid_string_test "zero capacity"
```

Ver [decisión de compatibilidad y evidencia](../docs/runtime-compatibility/p1-e-steamid-count.md).
