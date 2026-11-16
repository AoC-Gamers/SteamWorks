# P1-C: creación controlada de detours e inicio fallido

Fecha: 2026-11-14. Rama: `develop`. Base
`46e3ee05fc29ad090a1d6f7b1cb5e3dca9855350`, conservando P1-A/P1-B y documentación
locales. Sin commit, release ni despliegue. Corrección de H02 implementada y
verificada por unidad; aceptación de parcheo/ABI en servidor pendiente.

## Contrato real de CDetour

Se revisaron [detours.cpp](../../extension/CDetour/detours.cpp) y
[detours.h](../../extension/CDetour/detours.h):

- `CreateDetour` devuelve null si la inicialización del objeto falla.
- `EnableDetour()` devuelve **void**. No permite comprobar un resultado booleano
  de instalación ni demostrar que se aplicó correctamente el parche.
- `IsEnabled()` refleja la inicialización interna del objeto; no es un acuse de
  que `EnableDetour` aplicó el parche. No se utilizó para fingir esa comprobación.
- `Destroy()` retira el parche si procede, libera el trampoline y elimina el
  objeto. No limpia por sí mismo la variable de trampoline que recibió del caller.

La guardia de P1-C actúa al recibir el resultado de la factoría. No corrige
fallos internos de asignación/parcheo que puedan ocurrir antes de que esa
factoría retorne, ni modifica el motor CDetour.

## Comportamiento antes y después

Antes, el constructor de [swgsdetours.cpp](../../extension/hooks/swgsdetours.cpp)
habilitaba cada resultado de creación sin comprobarlo y podía habilitar el
primer detour antes de crear el segundo. El wrapper de `InitSafe` solicitaba
hooks incluso si el original devolvía false.

Ahora el propietario:

1. Comprueba el resolver de gameserver, las dos direcciones y el motor SourcePawn.
2. Conserva la resolución existente: gamedata primero, exports como fallback
   para cada dirección ausente; cierra la referencia de biblioteca adquirida.
3. Crea shutdown y luego init, comprobando cada puntero antes de utilizarlo.
4. Si una creación falla, libera lo preparado, limpia ambos slots de trampoline,
   registra un diagnóstico y no habilita ningún detour del par.
5. Solo con ambos objetos creados llama a `EnableDetour`, primero shutdown y
   después init. Ese orden es una solicitud de instalación; su retorno void
   sigue sin demostrar éxito del parcheo.

`SteamWorksGSDetours::Shutdown()` retira primero init y después shutdown,
anula punteros de propietario y trampolines y se puede repetir. El destructor lo
reutiliza. Se impide copiar el propietario para no duplicar sus recursos.
No se añadieron natives, forwards ni comandos administrativos.

El callback `SteamGameServer_InitSafeDetour` conserva la entrada legacy de seis
argumentos, llama exactamente una vez al original y devuelve su resultado.
Después de false no consulta interfaces ni llama a `AddHooks`. Después de true
solo solicita hooks si existen los componentes y la interfaz adquirida no es null.
No se cambió el prototipo por la inicialización moderna del SDK.

El wrapper de shutdown conserva su comportamiento anterior: retirada de hooks
GameServer, reset de cachés y original. P1-C no rediseña esa secuencia.

## Política de fallo y límites de ciclo de vida

El par se trata como capacidad opcional: un fallo no se propaga como fallo de
carga de toda la extensión. Se registra una causa en el constructor; el wrapper
no añade logs repetidos por frame o por intento de inicio.

Esto cambia una ruta parcial: antes podía habilitarse solo shutdown si init no
se resolvía. Ahora un símbolo ausente desactiva el par completo. El diagnóstico
identifica los símbolos ausentes o la creación/precondición fallida.

**Desactivar este par no retira hooks instalados por otros componentes.** No es
un estado de cierre seguro global ni una solución a H10. La coordinación con
los pollers GameServer/GC, HTTP y callbacks corresponde a P3; requiere aceptación
antes de declarar soporte operativo para una ruta degradada. Tampoco se prueba
que todo fallo de inicialización del motor cancele trabajo de otros componentes:
la garantía de esta entrega es el posinicio del wrapper modificado.

`Shutdown()` cierra los recursos del propietario, no ejecuta el shutdown de
Steam. Los tests de repetición son secuenciales; no certifican cierre reentrante
o concurrente mientras se ejecuta un trampoline.

## Pruebas deterministas

[detours_lifecycle_test.cpp](../../tests/detours_lifecycle_test.cpp) incluye el
C++ de producción y su clase propietaria. El
[host de pruebas](../../tests/detours_test_host.h) sustituye símbolos, factoría,
operaciones de parcheo, SourceMod y llamadas originales. Las macros de callback
conservan la forma de argumentos para el test, pero no validan ABI.

| Grupo | Casos y propiedad comprobada |
| --- | --- |
| 14 construcciones | Éxito; fallo de cada creación; símbolos, biblioteca, engine o resolver ausentes; gamedata completa/parcial, config ausente y exports |
| Limpieza por construcción | Ningún enable antes de crear ambos objetos; rollback del primero si falla el segundo; referencias de biblioteca balanceadas; cierre doble y destructor sin doble destrucción |
| 1 reintento con nuevo propietario | El fallo previo no deja recursos/trampolines y permite crear/destruir un nuevo par |
| 8 invocaciones de inicio | Retorno y seis argumentos originales preservados; fallo con interfaz todavía visible; ausencia de componentes/interfaz; llamadas repetidas |
| 3 invocaciones de shutdown | Original ejecutado una vez y orden anterior conservado, con/sin componentes |

Son **26 escenarios nuevos** por arquitectura. El test de inicio repetido cuenta
una solicitud de hooks por callback exitoso: no demuestra la deduplicación real
de SourceHook ni la instalación atómica de sus tres hooks.

Se construyeron además dos probes contra copias del C++/header anteriores a
P1-C: el wrapper original falla la propiedad de no buscar interfaz/añadir hooks
tras false; el constructor original produce un fallo UBSan por llamada de
miembro sobre puntero null cuando la factoría falla. Son fallos de regresión
esperados dentro de ejecutables aislados, no crashes provocados en un servidor.

## Resultados y aceptación pendiente

| Nivel / prueba | Resultado |
| --- | --- |
| T06/U | Fallos de creación comprobados; sin desreferencia del null retornado |
| T07/U | Inicialización original fallida no busca interfaz ni solicita hooks |
| T08/U, alcance de wrapper | Argumentos, resultado y orden de shutdown preservados con dobles |
| T09/U | Preparación parcial revertida; cierre repetido y destrucción sin duplicación |
| x86_64 y x86 | 26 casos nuevos + 66 regresiones de stats pasan con UBSan: 92 por arquitectura |
| Headers reales | `swgsdetours.cpp` compila sin warnings como objeto ELF32 i386 |
| T24/B | Superficie SourcePawn y prototipo legacy sin cambios |
| CI Linux | Suite añadida antes del build; lint local aprobado, ejecución remota pendiente |
| T06–T09/I, T08/R y T12/R | Pendientes de servidor/módulos identificados y comprobación real de parcheo, init y cierre |

La compilación de la unidad usa los headers y dependencias identificados en P0.
Como en P1-A/B, el comando diagnóstico omite solo la opción de C
`-Wno-pointer-to-int-cast`, incompatible con C++ bajo `-Werror`; no modifica
AMBuild. No equivale a un build completo enlazado. El bloqueo del build normal
registrado en P0 sigue pendiente.

## Reproducción y evidencia

```bash
bash scripts/test-detours.sh
bash scripts/test-stats-natives.sh
```

Para x86, usar `TEST_CXXFLAGS=-m32` en un toolchain multilib. Se reutilizó el
contenedor P0 sin red y con fuentes en solo lectura salvo `.build/`, sin instalar
paquetes nuevos. El SDK se indica con `STEAMWORKS_SDK_DIR` o `sdk_158a/` por defecto.

El [registro JSON](p1-c/validation-results.json) identifica fuentes, baseline,
entornos, comandos y resultados. Los logs y objetos locales permanecen en
`.build/p1-c/`. Las instantáneas de fases anteriores se mantienen históricas.

Para revertir solo P1-C, retirar su diff en `swgsdetours.*`, suite/runner, paso
CI y documentación, conservando stats y documentación previas. No hubo despliegue
que revertir. No se aprobaron firmas, offsets, Windows ni el perfil binario legacy.
