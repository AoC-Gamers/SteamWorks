# SourcePawn: causa y corrección del fallo al destruir la VM

[P5-R2](p5-integration.md) repite esta validación e incorpora las correcciones
a los artefactos actuales. Las referencias al P5 original al final de este
informe se refieren a su captura histórica, que permanece intacta.

Fecha: 2026-11-14. Rama `develop`, cambios sin commit ni despliegue.
Se corrige la dependencia en una copia de pruebas, sin reemplazar bibliotecas
instaladas y sin modificar la API pública SteamWorks o SourcePawn.

## Causa demostrada

`sp::Environment` poseía los objetos creados como `SourcePawnEngine` y
`SourcePawnEngine2` mediante `unique_ptr<ISourcePawnEngine>` y
`unique_ptr<ISourcePawnEngine2>`. Esas interfaces no tienen destructor virtual.
La eliminación mediante el tipo base es comportamiento indefinido; en x64,
ASan observa una reserva de 264 bytes liberada con tamaño 8.

El programa mínimo `sourcepawn_environment_test.cpp` reproduce el error creando
y destruyendo únicamente el entorno. No incluye SteamWorks, CForward, SourceHook,
plugins ni módulos Steam. Reproduce tanto el fallo del binario precompilado usado
en T36 como el de una VM instrumentada compilada desde la revisión fijada sin
el parche de ownership. Por tanto, el fallo no procede del marshalling del ticket
ni de un cambio de firmas entre las cabeceras del harness y la VM.

En la revisión de fuentes fijada también se observó un destructor `Environment`
vacío: `delete` no efectuaba `Shutdown()` ni retiraba el entorno global, pese al
contrato documentado de `ISourcePawnEnvironment`. Quitar simplemente el `delete`
del harness dejaría recursos vivos y no sería una corrección.

## Corrección y alcance

[environment-ownership.patch](../../patches/sourcepawn/environment-ownership.patch)
usa `unique_ptr<SourcePawnEngine>` y `unique_ptr<SourcePawnEngine2>`. El destructor
fuera de línea ve los tipos completos y elimina mediante el tipo correcto.
`APIv1()` y `APIv2()` siguen devolviendo las mismas interfaces públicas.

El destructor del entorno llama a `Shutdown()` cuando todavía es el entorno
registrado. Si el llamador ya lo cerró explícitamente, no vuelve a liberar pools.
Se comprueba la existencia del watchdog al cerrar una inicialización parcial.
Las cabeceras públicas y los métodos virtuales de las interfaces no cambian.

La corrección se guarda como parche dentro de SteamWorks. El constructor copia
la revisión `11b22edb634b9764d19fd28699e03289cfd18520` a una salida nueva en
`.build`, verifica los archivos cambiados y registra sus hashes antes/después.
El checkout de SourceMod/SourcePawn original permanece limpio y la VM precompilada
conserva su hash anterior; continúa siendo un control negativo válido.

## Instrumentación sin supresiones

Se usa el modo ASan de SourcePawn con Clang 14, aplicándolo a la VM y dependencias
compiladas, además de instrumentar el harness. La liberación con tamaño se activa
explícitamente con `-fsized-deallocation`, para que Clang detecte el defecto de la
misma forma que el control original compilado con GCC.

El parche de build independiente habilita RTTI: el modo habitual de SourcePawn
usa `-fno-rtti`, incompatible con la comprobación de vptr de los harnesses UBSan
en este límite de biblioteca. Se habilitan los metadatos necesarios en la VM
de prueba en lugar de desactivar esa comprobación. El build se limita a la VM;
compilador, shell y verifier no son necesarios para probar su propiedad.

Los ejecutables de prueba usan `-fno-pie -no-pie`. En las pruebas diagnósticas,
el runtime ASan 14 con PIE entró en una repetición de `DEADLYSIGNAL` en este host
con alta entropía de mmap. El ejecutable no PIE evita ese episodio en las
ejecuciones registradas; esto no desactiva ASLR del sistema ni la instrumentación.
La VM continúa siendo una biblioteca PIC. No se presenta ese diagnóstico de
arranque como la causa del fallo de eliminación, que tiene su control independiente.

Opciones explícitas de aceptación:

```text
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:alloc_dealloc_mismatch=1:new_delete_type_mismatch=1
LSAN_OPTIONS=exitcode=23
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
```

No se usan archivos de supresión, desactivación de fugas, liberación sin tamaño,
`no_sanitize`, anulación de operadores de memoria ni omisión de `delete`/`dlclose`.

## Resultados

En Linux x86_64 y x86:

- Build instrumentado de la VM con warnings como errores.
- 16 ciclos por arquitectura, alternando `delete` directo y `Shutdown()` seguido
  de `delete`; el entorno global queda vacío y se puede crear el siguiente.
- 227 comprobaciones de autenticación por arquitectura con CForward real,
  receptores SourcePawn sintéticos y VM instrumentada, sin fallos ASan/LSan.
- El control x64 sin arreglo de ownership reproduce `new-delete-type-mismatch`
  con la misma instrumentación. No se convierte su fallo esperado en aceptación.
- Cabeceras públicas SourcePawn idénticas antes/después del parche; inventario
  público SteamWorks y fuente de producción sin cambios respecto de T36.

La regresión conjunta pasa sus diez suites en ambas arquitecturas: se conserva
GCC/UBSan para las suites nativas y la prueba normal de prefijos, y se usa Clang
con ASan/UBSan para las dos suites de VM. El ejecutor registra compilador y flags
por suite; `VM_TEST_CXX` y `VM_TEST_FLAGS` seleccionan explícitamente su toolchain.
La prueba de prefijos sigue sin certificar la memoria interna de Steam.

La barrida diagnóstica adicional que imponía Clang/ASan uniformemente a todo el
proyecto no se acepta como matriz: encontró dos declaraciones HTTP sin `override`
que Clang rechaza con `-Werror`, y LSan informó una reserva al descargar una
referencia SDK en el test de prefijos x86. Esos logs se conservan, sin supresiones
ni atribuir esos resultados al ownership de la VM. Las pruebas del arreglo usan
la VM instrumentada en ambos casos y mantienen activas sus comprobaciones.

Resultados, revisiones, hashes de binarios, controles y logs:
[validation-results.json](sourcepawn-vm-asan/validation-results.json).
Los resultados T36/P5 previos se conservan como evidencia histórica.

## Reproducir

Dentro del toolchain preparado y con las dependencias disponibles:

```bash
SOURCEMOD_DIR=/ruta/sourcemod \
SPCOMP=/ruta/sourcemod-runtime/scripting/spcomp64 \
VM_TEST_BUILD_DIR="$PWD/.build/mi-vm-asan-x64" \
AMBUILD_PYTHON=/ruta/venv/bin/python \
AMBUILD_BIN=/ruta/venv/bin/ambuild \
  bash scripts/test-sourcepawn-vm-asan.sh
```

Para x86, añadir `VM_TEST_ARCH=x86` y elegir otra salida. El compilador del SMX
puede seguir siendo `spcomp64`. El runner construye, verifica y ejecuta ambas
pruebas sin acceder al servidor. Falla si la revisión no coincide, el checkout
tiene cambios o la salida ya existe. `test-integration.py` dispone de la opción
explícita `--with-sourcepawn-environment` para añadir la prueba mínima a las suites.

Para el control negativo, `build-test-sourcepawn.sh` acepta
`VM_TEST_VARIANT=baseline`, conservando instrumentación, RTTI y scope de build,
pero omitiendo el parche de ownership. El runner de aceptación rechaza esa variante.

## Límites y reversión

La VM compilada es un artefacto de pruebas, no una actualización instalada de
SourceMod. No se extiende esta validación a otro commit de SourcePawn, a Windows,
a todo el runtime SRCDS ni a tickets reales. La fuente de producción de SteamWorks
y su binario T36 permanecen iguales; los ZIP P5 tampoco se modifican.

Para retirar la corrección de pruebas, dejar de seleccionar esa VM aislada; no
restaurar una biblioteca sobre una instalación en ejecución. El control negativo
precompilado sigue disponible sin cambios. Una eventual actualización del servidor
requiere una entrega y validación propias, fuera de esta tarea sin despliegue.
