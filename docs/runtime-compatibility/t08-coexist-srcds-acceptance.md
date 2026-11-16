# T08: reinicialización aceptada con SourceTVSupport

El candidato `82347b99d5130bd411cdc0b35d215dc87fc82d394a2252ccd83d5d4940b6ca5a`
pasa dos reinicializaciones completas y el cierre final en SRCDS Linux x86,
con SourceTVSupport y los consumidores habituales de `competitive_dev`.
LinuxGSM inició la copia SourceMod aislada en 27015 con los flags y la
configuración de juego habituales. No se añadió contraseña.

## Ciclo real del motor

Después del arranque frío se ejecutaron `map c2m2_fairgrounds` y
`map c2m1_highway`. Las dos partidas nuevas conservaron PID **58894**, start ticks,
hashes de los módulos mapeados y contadores del harness. Hubo una sola instalación
del perfil, tres init con resultado original verdadero y tres shutdown originales
contando el arranque y el `quit` final.

Cada reinicialización produjo parada de consumidores, una nueva generación de
hooks, reactivación del runtime y exactamente un forward público de conexión.
El estado final de cada ciclo fue `loaded=1 connected=1`, con los tres hooks
GameServer tipados reinstalados. No hubo errores SourceMod ni callbacks duplicados
en el caso observado.

La [inspección posterior de los hooks](t08-coexist-srcds/detour-ownership.json)
confirmó el reparto previsto: el `CALL` del motor entra en SteamWorks, el export
compartido de init sigue entrando por el puente de SourceTVSupport y el shutdown
entra en SteamWorks. Se preservaron ambos participantes en vez de retirar el
consumidor que corrige el query port.

## HTTP y petición pendiente

Antes y después de cada reinicialización pasaron ocho casos HTTP: texto, binario,
respuesta vacía, error HTTP, timeout, TLS inválido, destino inaccesible y streaming.
Un probe adicional tras liberar la respuesta antigua elevó el total a **25
completions correctas y cero fallos**. Los tres casos de streaming entregaron
18 bytes válidos, tres callbacks de headers y ambas celdas del contexto.

Para el segundo shutdown, un harness conservó un handle de petición entre mapas.
La fixture confirmó la llegada de `/hold` antes de iniciar la partida nueva y
retuvo su respuesta hasta después de recuperar el runtime y completar los ocho
casos HTTP posteriores. La retención duró **31,311 segundos**, por debajo del
timeout de 60 segundos aceptado por el SDK.

Tras liberar esa respuesta se observó el juego durante siete segundos y se
completó una petición nueva: no llegó ningún callback de la petición antigua.
El plugin conservó su handle a través de los dos `OnMapEnd` y lo cerró al final
sin errores. No se invocaron getters sobre el request invalidado: las excepciones
que lo protegen siguen activas. Es una aceptación de esta cancelación real y su
ventana de observación, no de todas las carreras posibles de generaciones antiguas.

## Salida y restauración

`quit` completó el desmontaje sin SIGSEGV, SIGALRM ni nuevos cores. La sesión
cliente de aplicación se cerró una sola vez durante el unload final, con el pipe
de gameserver en cero; no se cerró durante las dos reinicializaciones.

El [DEV habitual](t08-coexist-srcds/cleanup.json) quedó iniciado mediante LinuxGSM
y conectado a Steam. Selector, módulo instalado y cuatro hashes protegidos son
los originales. El laboratorio y las fixtures locales se retiraron; producción
no se intervino.

## Alcance de aceptación

T08 pasa para este ELF y la pareja binaria Linux x86 identificada.
El [fallo anterior](t08-reinit-srcds-acceptance.md) de `cd42f40c…` se conserva;
no se cambia retrospectivamente su resultado. La [corrección local](t08-init-coexistence.md)
con sus 14 suites y sanitizadores corresponde al mismo producto probado aquí.
En este seguimiento solo cambiaron los harnesses de laboratorio y la documentación.

No se autenticó un humano ni se transfieren al nuevo ELF las aceptaciones previas
de identidad/stats. GC, descarga real de la extensión en caliente, owner unload
con una petición pendiente, desconexión controlada de Steam, otras carreras y
operación sostenida siguen fuera de este ensayo. P5 no obtiene aceptación global.

[Resultados y hashes](t08-coexist-srcds/results.json),
[auditoría de cores](t08-coexist-srcds/core-audit.json) y
[contrato público/origen del binario](t08-coexist-srcds/api-source-check.json).
El archivo de evidencia local está en `.build/t08-coexist-srcds/review/`;
no incluye claves TLS, comandos privados, módulos SDK ni cores. Los paquetes
anteriores permanecen intactos. Rama `develop`, sin commit ni publicación.
