# BeginAuthSession y diagnóstico de la extensión

Seguimiento: la [aceptación real en SRCDS](auth014-srcds-acceptance.md) registra
el candidato R3, la corrección de enlace de tier0 y un evento real de autenticación.
Este documento conserva el checkpoint previo y su hash original.

Actualización: el [adaptador tipado 014](begin-auth-session-014.md) separa el
contrato de autenticación del contrato 015 de natives. Este documento conserva
su checkpoint anterior; los resultados nuevos están en el informe del adaptador.

Fecha local: 2026-11-14 (America/Santiago). `develop`, sin commit ni despliegue.
Se revisó la ruta que no emitió eventos en el
[ensayo con jugador real](srcds-acceptance-retry.md) y se añadió una convar de
trazado. **Esta compilación de diagnóstico no se ha cargado en SRCDS.** DEV
conserva su extensión anterior y no se reinició durante esta revisión.

## Ruta encontrada

La evidencia supera la comparación inicial de literales:

1. En el engine identificado por P4, `SteamInternal_Init_SteamGameServer` en RVA
   `0x202470` pasa el literal `SteamGameServer014` de `0x2b54c0` a
   `SteamInternal_FindOrCreateGameServerInterface`. Las relocaciones de las dos
   llamadas identifican `SteamGameServer_GetHSteamUser` y ese factory.
2. `CSteam3Server::NotifyClientConnect`, RVA `0x201cc0`, separa los primeros ocho
   bytes de identidad del paquete y llama en `0x201d9d` a la ranura virtual
   `0x68 / 4 = 26`, con ticket, longitud y SteamID. La ruta analizada sí usa la
   operación `BeginAuthSession`; no se deduce su ausencia de un contador cero.
3. El contrato de la extensión es `SteamGameServer015`. `SH_ADD_HOOK` instala
   `BeginAuthSession` en el objeto devuelto para esa versión, no en el objeto
   que solicita el engine.
4. Un proceso x86 separado solicitó ambos objetos con un usuario/pipe local y
   comprobó que objeto, vtable y función de la ranura 26 son distintos. Se
   repitió con los dos steamclient.so mapeados durante el ensayo anterior. Ambos
   probes terminaron con código 0. No llamaron a métodos de autenticación,
   `InitGameServer`, LogOn ni leyeron tickets o cuentas reales.

Esto identifica un desajuste concreto entre el objeto observado y la ruta del
engine. El éxito de `SteamWorks_OnValidateClient` no lo contradice: esos forwards
se alimentan de callbacks Steam y no dependen del hook virtual sobre 015.

La evidencia es estática más un probe de objetos separados; no se adjuntó un
depurador al servidor ni se capturó el puntero efectivo del engine durante una
conexión. La corrección de transporte todavía necesita un adaptador auditado
para el contrato del engine y una nueva aceptación con jugador. No se sustituye
la versión 015 del resolver ni se acepta 014 mediante un cast incompatible.
El marshalling de T36 permanece intacto y conserva sus pruebas deterministas.

[Hashes y resultados](auth-debug/results.json), [evidencia binaria](auth-debug/binary-evidence.json)
y [probe reproducible](../../tests/diagnostics/gameserver_interface_probe.cpp).
El probe mantiene steamclient mapeado hasta que termina su proceso, después de
liberar usuario y pipe: el intento inicial de hacer `dlclose` terminó con 139
tras emitir los resultados, por lo que se descartó esa ejecución. No se suprimió
ninguna comprobación ni se atribuye ese fallo al servidor o a SourcePawn.

## Convar

`steamworks_debug` es una máscara entera, predeterminada a **0**, con rango 0–15.
Se registra con Metamod y se retira al desmontar la extensión. No se alteran los
47 natives, las firmas de forwards ni el include público.

| Valor | Categoría | Eventos |
|---|---|---|
| 0 | Apagado | No crea el directorio o archivo de debug ni escribe eventos |
| 1 | Lifecycle | Carga, descarga, inicio y parada del runtime |
| 2 | Interfaces | Versión rechazada y resolución exitosa del gameserver |
| 4 | Hooks | Espera, instalación por método, estado instalado y retirada |
| 8 | Autenticación | Entrada al hook, omisión, resultado del despacho y callbacks públicos |
| 12 | Hooks + autenticación | Máscara recomendada para investigar esta ruta |
| 15 | Todas | Incluye lifecycle e interfaces |

Con esta compilación cargada, desde la consola del servidor:

```text
steamworks_debug 12
```

El cambio emite un snapshot de hooks y receptores existentes; no necesita
reinstalarlos ni adquirir otra interfaz. Para detener el trazado:

```text
steamworks_debug 0
```

El archivo se encuentra en `<sm_basepath>/logs/steamworks/debug.log`; el anterior
es `debug.previous.log`. Se rota antes de superar 1 MiB por archivo. El backend
abre en append, reutiliza el archivo y hace flush por evento. Al apagar o
finalizar se cierra. Los mensajes del intento actual pueden coexistir con los
anteriores; revisar timestamps y eventos de lifecycle.

Formato de ejemplo, ilustrativo y no extraído de un SRCDS con este build:

```text
[hooks] event=gameserver_state enabled=1 target=1 waiting=0 restart=1 logon=1 auth=1 version=SteamGameServer015
[auth] event=auth_state enabled=1 attached=1 receivers=1
[auth] event=begin_auth_enter enabled=1 target=1 receivers=1 bytes=...
[auth] event=begin_auth_dispatch result=0 cells=...
[auth] event=validation_callback source=ValidateAuthTicketResponse result=0
[auth] event=validation_dispatch forward=current receivers=1
```

`begin_auth_dispatch result=0` significa que el despacho SourceMod tuvo éxito;
no es el resultado del método Steam original. El hook continúa siendo observador
(`MRES_IGNORED`). Los skips distinguen entrada inválida, reserva fallida, push
fallido y receptor/runtime inactivo. La categoría de interfaces no imprime la
cadena arbitraria rechazada; solo la familia y versión esperada.

No se registran tickets, payloads, tokens, AccountID/SteamID, nombres de jugadores,
IP ni direcciones de memoria. Se permiten longitud, número de receptores,
resultados y estados booleanos. No se registra una línea por frame al esperar
la interfaz. Un fallo de apertura, rotación o escritura emite un solo error
SourceMod y desactiva el archivo hasta recargar la extensión; no cambia el
resultado de autenticación. Las categorías se filtran antes del formateo o I/O.

## Validación y límites

- 32 comprobaciones de logger: apagado, cada bit, combinaciones, creación del
  directorio, append, reapertura, rotación repetida, shutdown y fallo de filesystem
  sin reintentos ni spam. También pasan con ASan/LSan en x86.
- 53 comprobaciones de hooks: incluyen trazas escalares de entrada/despacho,
  snapshot sin nueva adquisición y apagado, además de las guardas de ownership.
- Nueve suites de integración pasan en host y x86, con inventario público idéntico.
- 227 comprobaciones del marshalling con CForward y VM instrumentada por
  arquitectura; ASan/LSan y comprobaciones de tipos/tamaños permanecen activos.
- Build Linux i386 con warnings como errores en el código propio. ConVar y sus
  dependencias tier1 se recompilan como PIC desde el SDK fijado, con sus headers
  tratados como externos; no se parchean headers ni se enlazan objetos legacy
  que introducían `DT_TEXTREL`. El ELF final no tiene text relocations.
- Workflow incorpora el test de debug. CI remota, Windows y macOS no ejecutados.

La prueba de registro/cambio/retirada de la convar en un host SRCDS real sigue
pendiente. Los tests locales del backend y el build no sustituyen esa aceptación.
El nuevo binario tiene hash propio y no hereda las pruebas del P5-R2 congelado;
los artefactos anteriores se conservan. El fallo HTTP streaming tampoco se
corrige en este cambio.
