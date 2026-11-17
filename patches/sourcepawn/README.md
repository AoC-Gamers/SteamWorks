# SourcePawn: VM aislada para pruebas con ASan

Base admitida: `11b22edb634b9764d19fd28699e03289cfd18520`, el submódulo
SourcePawn de las dependencias SourceMod utilizadas por la VM de pruebas.
Estos parches se aplican solo a una copia en `.build`, nunca al checkout original
ni a una instalación.

- `environment-ownership.patch`: conserva los motores mediante sus tipos
  concretos y completa el cierre del entorno desde su destructor. No añade un
  destructor virtual a las interfaces públicas ni cambia cabeceras `include/`.
- `test-vm-only.patch`: limita el build a la biblioteca VM y habilita RTTI para
  comprobar vptr desde los harnesses UBSan. No modifica sus algoritmos ni desactiva
  sanitizadores. Evita construir compilador, shell y verifier ajenos a esta prueba.

`tests/helpers/build-test-sourcepawn.sh` comprueba revisión y estado del checkout,
rechaza sobrescribir salidas y conserva hashes antes/después de todos los archivos.
La variante `baseline` omite únicamente la corrección de ownership y permite
reproducir el fallo con las mismas opciones de instrumentación y build.

El flujo de aceptación es `tests/runners/test-sourcepawn-vm-asan.sh`: necesita un
compilador Clang con runtime ASan, AMBuild y las dependencias ya presentes.
`tests/integration/sourcepawn-vm.Dockerfile` prepara un toolchain local a partir
de una imagen base explícita; registra su digest, pues los paquetes se obtienen
del repositorio Debian. El flujo de compilación/pruebas posterior es sin red.

Este parche es una corrección revisable de la dependencia de pruebas. No supone
que se haya corregido o reemplazado un binario precompilado del servidor. Véanse
[causa, pruebas y reproducción](../../tests/integration/README.md).
