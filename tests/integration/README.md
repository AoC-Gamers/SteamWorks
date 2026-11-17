# Integración offline: SourceHook y SourcePawn

Estas suites usan componentes reales del host sin iniciar SRCDS ni conectar a Steam.
`auth_binding_test.cpp` requiere SourceHook de Metamod y objetos Steam simulados
014/015. `auth_forward_test.cpp` carga una VM real y compila el receptor
`../fixtures/sourcepawn/auth_forward_receiver.sp`; comprueba binarios y reglas de
agregación reales. `sourcepawn_environment_test.cpp` repite creación/destrucción
de entornos sobre una VM instrumentada de arquitectura coincidente.

```bash
make test TEST_OUTPUT=.build/test-results/con-vm \
  TEST_OPTIONS="--with-auth-binding --with-auth-forward --with-sourcepawn-environment"
```

Configura `MMSOURCE_DIR`, `SOURCEMOD_DIR`, `SPCOMP` y `SOURCEPAWN_VM`. Con x86 usa
un compilador/runtime ASan de 32 bits compatible con la VM. `VM_TEST_CXX` y
`VM_TEST_FLAGS` permiten seleccionar ese toolchain sin cambiar las suites nativas.
ASan/LSan/UBSan deben permanecer activos: no usar supresiones para aceptar un fallo.

`make test-sourcepawn-vm-asan` construye y comprueba una VM diagnóstica
aplicando los parches versionados mediante los backends
`tests/helpers/build-test-sourcepawn.sh` y `test-sourcepawn-vm-asan.sh`. El Dockerfile de este directorio describe un toolchain local cuya
imagen y revisión deben registrarse en cada ejecución. Indica una imagen de
toolchain preparada mediante `--build-arg BASE_IMAGE=<imagen>` al construirlo;
el Dockerfile no presupone una etiqueta local de la instalación del mantenedor. El objeto derivado debe
destruirse por su ruta correcta; esa corrección no equivale a desactivar la
comprobación de tipos/tamaños de ASan.

Las salidas pertenecen a `.build/`. Ni esa biblioteca ni los SMX generados se
instalan en el servidor o se incluyen en un paquete de producto. Un éxito offline
no acredita autenticación de una cuenta ni la VM distribuida por SourceMod.
