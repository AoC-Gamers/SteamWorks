# T08: SIGSEGV al salir de SRCDS x86

El binario corregido `cd42f40ca461c5d78ed9ebca73a485643bc4bc4946cf5168c3089b18c713847b`
completa tres arranques/HTTP/`quit` en `competitive_dev`, con LinuxGSM, árbol
SourceMod aislado y puerto 27015, sin SIGSEGV, nuevos cores ni errores de SourceMod.
Uno de los procesos también cambia a `c2m2_fairgrounds` manteniendo Steam y el
mismo PID. El servidor DEV original quedó restaurado y conectado a Steam;
sus cuatro archivos protegidos conservan sus hashes. Producción no se intervino.

## Causa y corrección

Los cores previos del candidato P5-R5 y del baseline comparten la misma firma:
un worker de `sdk32/steamclient.so` accede a memoria ya desmapeada de
`crashhandler.so`. El manejador instalado para SIGSEGV también apunta a ese módulo
desmapeado. El main thread espera el cierre del SDK; no debe confundirse con el
worker que provoca la señal original.

El trazador temporal identifica la llamada real a `SteamAPI_Init` en
`CServerGameDLL::DLLInit`, retorno en `server_srv.so + 0x6b7117`. Al cerrar
`SteamGameServer_Shutdown`, el pipe de gameserver pasa a cero pero el de cliente
permanece activo. El loader finaliza `libsteam_api.so` y desmapea `crashhandler.so`
antes de que termine el worker. Tanto el binario original como el candidato
anterior dejan abierta esa sesión de aplicación; compartir el fallo no bastaba
por sí solo para atribuirlo.

La extensión registra que **la llamada original de shutdown del gameserver ya
terminó**. En `SDK_OnUnload`, después de detener sus consumidores y antes de
liberar la biblioteca del SDK, cierra la sesión cliente restante mediante
`SteamAPI_Shutdown`, únicamente si se observó aquel shutdown y el pipe de
gameserver sigue en cero. Un arranque correcto cancela la observación anterior.
No se enumeran ni se fuerzan cierres de pipes ajenos.

Un cambio de mapa no ejecuta esta finalización. Una descarga de extensión con
gameserver activo, una carga fallida sin shutdown observado y una observación
cancelada por reinicialización tampoco cierran la sesión. Los casos de política
se prueban de forma determinista; la aceptación de unload real en caliente y
reinicialización completa en un mismo proceso sigue pendiente.

Se descartaron hooks de cierre de GameDLL/VSP: esta instalación carga Metamod en
modo VSP y descarga las extensiones antes de `DLLShutdown`. Un hook de `VSP::Unload`
deja su dispatcher en la pila mientras el original descarga SourceHook y la propia
extensión. Ese ensayo produjo una firma distinta, fue rechazado y no forma parte
de la solución. El cierre final usa el callback normal de descarga de SourceMod.

No se ignoran señales, se fijan módulos en memoria ni se desactivan comprobaciones.
`SteamAPI_Shutdown` conserva el cierre del SDK, incluido su intento original de
`BShutdownIfAllPipesClosed`; no se añade otro cierre global redundante.

## Evidencia y límites

- [Resultados y hashes](t08-shutdown/results.json): ciclos `fixed-7`, `fixed-8`
  y `fixed-9`, con identidad del ELF32 realmente mapeado y HTTP/contexto correctos.
- [Firma original filtrada](t08-shutdown/baseline-signal-context.json),
  [llamador de init](t08-shutdown/api-init-callers.log) y
  [orden del loader](t08-shutdown/loader-events.json). No se distribuyen cores,
  módulos propietarios del SDK ni líneas de comando privadas.
- [Validación integrada](t08-shutdown/integration-results.json): 13 suites x86,
  API pública conservada, CForward/VM y binding 014. La política del finalizador
  añade 10 comprobaciones; la coordinación tiene 48 y los wrappers 26 escenarios.
- [ASan/LSan/UBSan nativos](t08-shutdown/asan-lifecycle.log), sin supresiones;
  las pruebas de VM instrumentada siguen aisladas del SRCDS instalado.
- [Auditoría de cores](t08-shutdown/core-audit.json) y
  [restauración del DEV](t08-shutdown/cleanup.json). Los tres ensayos finales no
  usaron `LD_PRELOAD` ni `LD_DEBUG`; los originales de systemd se conservaron.

El cambio corrige la salida del nuevo candidato en la pareja Linux x86
identificada. La [aceptación posterior de reinicialización](t08-reinit-srcds-acceptance.md)
falla con SourceTVSupport: el init efectivo no vuelve al observador de SteamWorks.
T08 queda fallido en ese caso; P5 no obtiene aceptación global. Las aceptaciones humanas previas de autenticación,
stats y streaming pertenecen a `bc76b00d…`, no se transfieren a este ELF nuevo.
El baseline archivado permanece intacto y conserva su fallo de salida.

Los artefactos de revisión de este seguimiento van en `.build/t08/review/`, con
manifiesto/checksums y reversión al baseline capturado. Los ZIP P5-R5 no se
sobrescriben. Todo continúa en `develop`, sin commit ni publicación.

La [aceptación posterior del ELF `82347b99…`](t08-coexist-srcds-acceptance.md)
pasa dos reinicializaciones con SourceTVSupport, HTTP retenido y cierre final.
Los resultados y límites anteriores siguen atribuidos a sus respectivos binarios.
