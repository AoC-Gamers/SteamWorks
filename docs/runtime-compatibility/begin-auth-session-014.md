# BeginAuthSession: enlace tipado de SteamGameServer014

Seguimiento: la [aceptación real en SRCDS](auth014-srcds-acceptance.md) registra
el candidato R3, la corrección de enlace de tier0 y un evento real de autenticación.
Este documento conserva el checkpoint previo y su hash original.

Fecha local: 2026-11-14 (America/Santiago). Rama `develop`, cambios sin commit ni
despliegue. Se corrige el enlace identificado en la
[revisión de autenticación](auth-debug.md). **El objetivo de compilación,
validación y entrega es Linux x86 (i386).** La corrección pasa las pruebas locales;
su aceptación con Steam y SRCDS reales permanece pendiente. No se reinició ni
modificó el gameserver en esta tarea.

## Cambio

El engine del perfil `l4d2-23990100-linux-x86` solicita `SteamGameServer014` y
llama a `BeginAuthSession` en la ranura 26. La extensión instalaba ese hook en
el objeto 015 que usa para sus natives. Las dos versiones pueden entregar
objetos y vtables diferentes, como mostraron los probes anteriores.

Ahora hay dos contratos separados:

| Uso | Tipo y versión | Adquisición |
|---|---|---|
| Natives y hooks existentes de restart/logon | `ISteamGameServer`, SDK 1.58a, `SteamGameServer015` | Resolver existente |
| Hook `BeginAuthSession` | `steamworks::ISteamGameServer014`, prefijo auditado del SDK 1.55 | `SteamInternal_FindOrCreateGameServerInterface(SteamGameServer_GetHSteamUser(), "SteamGameServer014")` |

El segundo usa el mismo factory y usuario del engine, sin convertir un puntero
015 a 014 ni reinterpretar 014 como el SDK actual. El prefijo tiene únicamente
las ranuras 0–26 que necesita SourceHook para localizar el método; no es una
implementación completa de 014 ni habilita sus métodos restantes. No añade
herencia desde 015, datos ni destructor virtual. Su destructor protegido deja
explícito que el objeto es prestado y no debe destruirse desde este adaptador.

La fuente auditada es `sdk_155/public/steam/isteamgameserver.h` del repositorio
`AoC-Gamers/Steamworks-SDK`, revisión
`a7aaa2754d9cda244d0cd9d7523c1144a1c472d2`. El
[fixture de declaraciones](../../tests/fixtures/gameserver014-prefix.json)
preserva su hash y las 27 firmas ordenadas. El SDK de compilación y enlace de la
extensión continúa siendo 1.58a. No se modificaron los SDKs ni SourceHook.

El registro incorpora un rol interno independiente `GameServerAuth`, que solo
acepta 014. La clave opcional de gamedata es
`SteamGameServerAuthInterfaceVersion`; cualquier versión distinta se rechaza
antes de consultar usuario o factory y el rechazo permanece hasta reset. La
clave existente `SteamGameServerInterfaceVersion` sigue siendo exclusivamente
015: cambiarla a 014 no activa este adaptador ni modifica el contrato de natives.
La carga de la extensión conserva la puerta del perfil binario verificado.

## Ciclo de vida y API

La caché de 014 es independiente de la de 015. Un runtime detenido oculta incluso
un puntero cacheado; `Reset` invalida ambos sin destruir objetos Steam. La ausencia
del usuario, del factory o de la interfaz produce `nullptr`, sin fallback a 015.

El grupo espera a tener ambos destinos antes de instalar sus tres hooks. Si
falla una instalación, retira las anteriores y queda deshabilitado hasta restart.
Conserva por separado el destino 015 y el 014; compara ambos al volver a enlazar.
Así, una sustitución de 014 vuelve a instalar el hook aunque 015 no haya cambiado.
El desmontaje elimina los IDs instalados antes de limpiar esos destinos y antes
del reset del resolver; no consulta un nuevo objeto para retirar hooks antiguos.

No cambian el nombre, registro, firma ni parámetros de
`SteamWorks_BeginAuthSession`, su marshalling de bytes compactos ni el modo
`MRES_IGNORED`. El original conserva su llamada y resultado. Tampoco cambian
los 47 natives, el include público o los forwards declarados. El inventario
integrado sigue coincidiendo con P0.

`steamworks_debug` conserva sus máscaras. Su snapshot muestra
`nativeVersion=SteamGameServer015 authVersion=SteamGameServer014`, y el evento de
instalación identifica 014. El resolver añade `auth_server_resolved` cuando
adquiere ese objeto. No se registran tickets, tokens, AccountID/SteamID ni direcciones.

## Pruebas deterministas

- **216 comprobaciones del resolver**: cachés independientes, rechazo de 013/015/016
  como contrato de auth, ausencia de usuario/factory, interfaz temporalmente
  ausente, rechazo persistente y separación del registro 015 de natives.
- **Prefijo ABI fijado**: se comparan las 27 declaraciones con el fixture auditado.
  Un intento de conversión implícita 014→015 debe fallar al compilar. Los asserts
  verifican tamaños de usuario, longitud, resultado y SteamID, la ausencia de
  destructor virtual y la incompatibilidad de punteros en ambas direcciones.
- **59 comprobaciones del owner de hooks** con SourceHook simulado: espera por
  014 aunque 015 exista, destino correcto por método, rollback de instalaciones
  parciales y rebind cuando cambia solo 014, además de teardown y reentrada.
- **18 comprobaciones con SourceHook real**, compilando sus fuentes fijadas:
  una llamada sintética a la ranura 26 entrega exactamente un forward y ejecuta
  exactamente una vez el original, cuyo resultado se conserva. Se comprueban
  bytes con ceros y bits altos, longitud y AccountID completo; un objeto 015 y
  una segunda instancia 014 no reciben el hook de la primera. Stop, sustitución,
  destrucción y unhook dentro del callback conservan el original y retiran el enlace.
- **Control negativo**: en una copia temporal de la implementación se restaura
  exclusivamente la declaración y el destino 015 del hook. La misma prueba
  pierde el evento del engine y falla por ese motivo. El original del repositorio
  permanece intacto. No se acepta un error de sanitizer como control negativo.
- **Marshalling de T36**: 227 comprobaciones en x86 con CForward y VM
  SourcePawn instrumentada, sin cambiar la corrección anterior ni sus checks.

La prueba de enlace usa SourceHook real con un receptor de forwards simulado y
objetos Steam sintéticos. La prueba de marshalling usa CForward y VM reales con
el transporte SourceHook simulado. Son alcances complementarios; no equivalen a
Steam/SRCDS completos ni a la recepción de tickets reales.

Diez suites integradas pasan para Linux x86. La extensión entregable es
exclusivamente ELF32 i386; el validador de paquetes rechaza ELF64 y EM_X86_64.
El helper de build especifica `--target x86`, y el runner integrado y la suite
de enlace usan `-m32` por defecto. La suite de enlace real se
repite en x86 con ASan/LSan y UBSan: sin errores observados, tanto al desmontar
normalmente como en el control negativo. Las comprobaciones de tipos/tamaños y
fugas permanecen activas, sin archivos de supresión. El código propio se compila
con warnings como errores; SourceHook se compila sin modificar, con su política
normal de warnings y los mismos sanitizers. El build Linux i386 pasa y conserva
el ELF sin `DT_TEXTREL`. CI remota no ejecutada.

## Reproducción

Con SDK 1.58a y el checkout de Metamod auditado ya disponibles:

```sh
MMSOURCE_DIR=/ruta/mmsource-1.12 TEST_CXXFLAGS=-m32 \
  bash scripts/test-auth-binding.sh
MMSOURCE_DIR=/ruta/mmsource-1.12 AMTL_DIR=/ruta/sourcemod/public/amtl \
  TEST_CXXFLAGS=-m32 python3 scripts/test-integration.py \
  --with-auth-binding --output .build/mi-validacion-auth014
```

La suite de enlace evita la desvirtualización del
caller sintético y llama explícitamente a la ranura 26 observada en el engine;
el código de producción utiliza la declaración tipada. El directorio de salida
incluye hashes de los inputs SourceHook y el control negativo. Las suites de VM
se reproducen con el procedimiento ASan de [T36](t36-auth-forward.md).

Los ensayos x86_64 ejecutados antes de fijar este alcance se conservan como
evidencia histórica de host. Sus binarios no se incluyen en la entrega ni
forman parte del objetivo de aceptación de L4D2.

Los [resultados y hashes](auth014/results.json) identifican esta compilación y
la evidencia nueva. Los ZIP P5-R2, el build de debug anterior y sus resultados
se conservan; ninguna aceptación se transfiere al nuevo binario. Siguen abiertos
el ensayo real del hook y el fallo HTTP streaming, que este cambio no toca.
