# T08: aceptación de reinicialización en el mismo proceso

**Resultado: fallido con los consumidores habituales.** El candidato
`cd42f40ca461c5d78ed9ebca73a485643bc4bc4946cf5168c3089b18c713847b`
conserva el PID `49349` al ejecutar dos partidas nuevas mediante
`map c2m2_fairgrounds` y `map c2m1_highway`, pero SteamWorks no vuelve a habilitar
su runtime. Se probó SRCDS Linux x86 en `competitive_dev`, iniciado por LinuxGSM,
árbol SourceMod aislado, puerto 27015 y configuración de juego habitual.

## Resultado observado

Antes de las transiciones pasan los ocho casos HTTP: texto, binario, vacío,
error HTTP, timeout, TLS inválido, destino inaccesible y streaming con contexto
completo. El estado es `loaded=1 connected=1`, ocho callbacks y cero fallos.

En las dos transiciones, el motor ejecuta el shutdown y vuelve a conectar a Steam.
El observador de shutdown de SteamWorks detiene consumidores y elimina los hooks.
Sin embargo, no recibe ninguna llamada posterior a su observador de init:
`loaded=0 connected=0` permanece después de ambas partidas nuevas y una petición
HTTP nueva devuelve `create_failed case=8`. El PID, los contadores del harness y
el hash del ELF mapeado se conservan: no son arranques fríos ni recargas simuladas.

## Frontera que impide la recuperación

La inspección de las entradas realmente parcheadas identifica:

- `SteamInternal_GameServer_Init`: salto a un puente ejecutable y de allí a
  `sourcetvsupport.ext.2.l4d2.so + 0xa1760`, símbolo local
  `Handler_SteamInternal_GameServer_Init`.
- `SteamGameServer_Shutdown`: salto al código de SteamWorks.

SourceTVSupport y SteamWorks intervienen la misma entrada de init. En esta carga,
el camino efectivo de SourceTVSupport no entrega el evento al observador de
SteamWorks, aunque el motor reconecta. El código consultado de SourceTVSupport
crea detours durante `SDK_OnLoad` y los habilita durante `SDK_OnAllLoaded`.
No se ha certificado una cadena cooperativa ni se ha cambiado ese orden en este
ensayo. La [evidencia del propietario](t08-reinit-srcds/detour-owner.json) fija el
salto, el módulo mapeado y el símbolo; la ausencia de init y el fallo HTTP acreditan
el efecto observable. No se fuerza disponibilidad por detectar solamente un pipe.

## Control descartado y errores de fixture

Se intentó un arranque mínimo retirando únicamente en la copia de laboratorio el
autoload de SourceTVSupport y dejando los harnesses como plugins. Con los flags
habituales de LinuxGSM, el SDK no puede enlazar el puerto de consulta 27115,
anuncia modo LAN y termina por `Alarm clock`/SIGALRM. El handler de SourceTVSupport
incluye precisamente una corrección del query port para ese caso. Por ello este
control no constituye una aceptación equivalente y no acredita reinicialización.

El primer gamedata de inspección contenía formato KeyValues inválido y produjo
un error propio de fixture, posterior al fallo de reinicialización. Su log se
conserva. El harness final utiliza direcciones de los exports verificadas por el
host contra identidad y segmentos del proveedor mapeado; solo lee sus entradas y
puentes desde la consola del laboratorio. Se compiló sin warnings con SourcePawn
1.12.0.7259. No modifica memoria ni añade natives públicos.

La petición pendiente prevista no se ensayó: el runtime ya había fallado al
recuperarse. El harness correspondiente se compiló, pero no se cargó. No se
presenta la compilación como aceptación de cancelación o generaciones antiguas.

## Restauración y siguiente paso

El proceso con consumidores habituales completó `quit` sin SIGSEGV. No aparecieron
nuevos cores desde la captura inicial; el SIGALRM del control mínimo se informa
por separado. La [aceptación previa del cierre](t08-srcds-shutdown.md) se conserva,
pero no acredita esta recuperación del runtime.

El [DEV original](t08-reinit-srcds/cleanup.json) quedó conectado a Steam, con su
selector, módulo y cuatro hashes protegidos intactos. Se retiraron el laboratorio
y las fixtures locales. No se modificaron C++, el binario candidato, los módulos
Steam, SourceTVSupport ni producción. La API pública coincide con P0; todo sigue
en `develop`, sin commit ni publicación.

Antes de repetir la aceptación hay que corregir la convivencia de los detours de
init, conservando argumentos, resultado original y comprobaciones del perfil.
Después deberán repetirse los dos ciclos, callbacks/HTTP/streaming y una petición
realmente pendiente durante shutdown. T08 queda **fallido en reinicialización**;
P5 continúa sin aceptación global. Los paquetes anteriores no se sobrescriben.

[Resultados y alcance](t08-reinit-srcds/results.json). Evidencia de revisión local:
`.build/t08-reinit/review/`, sin claves TLS, comandos privados, módulos SDK ni cores.

La [corrección local posterior](t08-init-coexistence.md) cambia la observación al
llamador del motor y pasa regresiones x86. Este ensayo sigue atribuido a
`cd42f40c…`; falta repetirlo con el nuevo binario.

La [aceptación posterior del ELF `82347b99…`](t08-coexist-srcds-acceptance.md)
pasa dos reinicializaciones con SourceTVSupport, HTTP retenido y cierre final.
Los resultados y límites anteriores siguen atribuidos a sus respectivos binarios.
