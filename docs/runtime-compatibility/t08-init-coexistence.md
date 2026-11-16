# T08: convivencia del init con SourceTVSupport

La corrección conserva la entrada compartida `SteamInternal_GameServer_Init` y
observa su llamada desde `CSteam3Server::Activate`. SourceTVSupport puede mantener
su detour y su corrección del query port; SteamWorks recibe el retorno del init
sin depender de qué consumidor capturó primero un trampoline del SDK.

La [aceptación anterior](t08-reinit-srcds-acceptance.md) de `cd42f40c…` sigue
fallida. El nuevo ELF32 es
`82347b99d5130bd411cdc0b35d215dc87fc82d394a2252ccd83d5d4940b6ca5a`.
Se compiló y probó localmente; no se cargó en SRCDS ni se operó `competitive_dev`
en esta tarea. La aceptación real con los consumidores habituales queda pendiente.

## Punto de intervención y contrato

En la pareja P4 identificada, el motor contiene un `CALL rel32` en RVA
`0x2018eb`, con relocation `R_386_PC32` en `0x2018ec` hacia el export del SDK.
Las instrucciones previas preparan los argumentos y las posteriores comprueban
AL. La evidencia binaria de esa ruta permanece en [EB01](p4/binary-evidence.md).

`VerifiedLifecycleInitCall` exige Linux i386, una única pareja con los build-id
admitidos, la dirección exacta del export (`steamBase + 0x1b430`), ambos rangos
ejecutables, los bytes del llamador alrededor del `CALL` y su destino relocado
igual a ese export. Un punto de llamada ya alterado, otro destino, un contexto
distinto o una pareja desconocida se rechazan antes de parchear.

`InitCallSite` sustituye únicamente los cinco bytes de esa instrucción. No copia
un prologue del SDK ni interpreta los saltos de otros consumidores. Su llamada
original apunta al export que el motor ya utilizaba; por tanto recorre el hook
vigente de SourceTVSupport, incluida su corrección de puerto y su post-init.
El wrapper conserva los seis argumentos y el resultado booleano; solo un resultado
original verdadero ejecuta `RuntimeStarted` después de terminar esa cadena.

El shutdown conserva su CDetour y sus comprobaciones de proveedor/RVA/prefijo.
Se prepara el conjunto antes de habilitarlo. Si falla preparar o escribir el
`CALL`, se revierte la carga y no se habilita el detour de shutdown. La escritura
comprueba `mprotect` y usa la misma política de páginas ejecutables de CDetour.
El unload restaura el `CALL` antes de cerrar la referencia al proveedor y no
reescribe la entrada que pertenece a SourceTVSupport. Una sustitución inesperada
del propio punto de llamada se detecta y no se sobrescribe; esto no certifica
cadenas de terceros que también intervengan ese `CALL` ni unload real en caliente.

La API pública, el SDK de compilación y SourceTVSupport permanecen sin cambios.
Los JSON documentales no habilitan el perfil ni sustituyen las comprobaciones.

## Pruebas deterministas Linux x86

`scripts/test-init-coexistence.sh` ejecuta:

- **142 comprobaciones de código x86 ejecutable:** propietario y wrapper de
  producción, `CALL` real en memoria y un proveedor/consumidor sintéticos. Cubre
  captura temprana y habilitación tardía como en el incidente, consumidor ya
  habilitado y consumidor cargado después; resultados verdadero/falso, seis
  argumentos, corrección del query port, una sola ejecución de cada participante,
  shutdown/reinit, descarga/recarga y restauración de bytes de cada propietario.
- **21 comprobaciones del selector de producción:** metadatos ELF sintéticos,
  ambas regiones ejecutables, módulos desconocidos/ambiguos, contexto truncado o
  alterado, destino incorrecto y aceptación del llamador aunque otro consumidor
  mantenga parcheada la entrada compartida del SDK.
- **Control negativo:** hacer que el `CALL` conserve el SDK sin pasar por el
  observador pierde el evento de éxito y falla la aserción de recuperación.

El conjunto también pasa con ASan/LSan/UBSan x86 y detección de fugas, sin
supresiones. Los wrappers/rollback tienen ahora 27 escenarios. La validación
integrada final pasa **14 suites**, incluido el contrato público P0 y las pruebas
de CForward/VM y binding 014. Una primera pasada detectó una declaración ausente
en un doble del perfil; se corrigió y la repetición completa pasó.

[Resultados, hashes y logs](t08-init-coexistence/results.json). Los harnesses
ejecutan código sintético: no certifican por sí solos el binario real de
SourceTVSupport, autenticación humana, stats o cancelación HTTP en SRCDS.

El siguiente paso es repetir las partidas nuevas en un mismo PID con este ELF,
los consumidores habituales de LinuxGSM y HTTP/streaming antes y después, incluida
una petición realmente pendiente durante shutdown. T08 no se marca aprobado por
esta corrección local y P5 sigue sin aceptación global. Los archivos y paquetes
de las aceptaciones previas permanecen intactos; todo sigue en `develop`, sin
commit, publicación ni despliegue.

La [aceptación posterior del ELF `82347b99…`](t08-coexist-srcds-acceptance.md)
pasa dos reinicializaciones con SourceTVSupport, HTTP retenido y cierre final.
Los resultados y límites anteriores siguen atribuidos a sus respectivos binarios.
