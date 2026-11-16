# T16: carga tardía y descarga/recarga de SteamWorks en SRCDS x86

2026-11-15. **T16 pasa en la ruta soportada probada**: carga tardía sobre un host
ya conectado a Steam y dos ciclos de descarga/recarga por consola, fuera de
callbacks y sin consumidores ni peticiones activas al descargar. Se usó el ELF
P5-R11 `839ff0751b04141031bdc6dffc48662d0804f52227f2c91015bd87c6b84cf192`.
API y fuentes de extensión intactas; `develop`, sin commit ni producción.

## Aislamiento y control independiente

`competitive_dev`, LinuxGSM `/app/l4d2server`, SRCDS x86, puerto 27015 y copia
`addons/sourcemod_acceptance`. Flags y CFG habituales conservados, sin añadir
contraseña. Los SMX originales de la copia se movieron a su recovery privado para
impedir autocarga de SteamWorks por un consumidor. Solo arrancaron el observador
y el inspector independientes, con las extensiones autoload habituales, incluida
SourceTVSupport. **No se prueba aquí la semántica de la pila completa de plugins.**

Se verificó el ZIP R11 y su payload se mantuvo inicialmente fuera de la búsqueda
de extensiones. Con el motor ya conectado a Steam se observó:

- SteamWorks sin mapear, biblioteca ausente y convar `steamworks_debug` ausente;
- pipes cliente **1** y gameserver **2**;
- bytes originales del CALL de init del engine y entrada de shutdown del SDK;
- entrada init del SDK ya modificada por SourceTVSupport.

El monitor no incluye `steamworks.inc` ni depende de SteamWorks. Sus dos SDKCalls
solo consultan los exports públicos `SteamAPI_GetHSteamPipe` y
`SteamGameServer_GetHSteamPipe`, con ABI estática x86 sin argumentos. El host
verifica primero hashes del SDK/engine, RVAs exportados y rangos ejecutables antes
de suministrar direcciones. Los hooks/entradas se leen mediante SourcePawn;
no se escribe código desde el monitor.

El campo `late=0` de sus trazas es el flag de carga **del plugin observador**,
que arrancó con el servidor. No representa el argumento de carga de la extensión.
La carga tardía se acredita con la ausencia inicial y la orden explícita de
carga después de inicializar/conectar el gameserver.

## Secuencia observada

1. Instalar el archivo verificado en la ruta de extensiones y ejecutar
   `sm exts load steamworks` sobre el host activo. La biblioteca y convar aparecen;
   CALL del engine y shutdown apuntan a SteamWorks. El init de SourceTVSupport no
   cambia y HTTP devuelve cuerpo/contexto/callback correctos.
2. Retirar el plugin de smoke tras completar y cerrar su petición. Descargar la
   extensión con el comando normal `sm exts unload <índice>`, sin código de fuerza
   ni ejecución desde un callback.
3. Confirmar biblioteca/convar ausentes y DSO realmente desmapeada. Los **5 bytes**
   del CALL y los **20 bytes** inspeccionados de shutdown coinciden exactamente con
   la captura inicial. Los 20 bytes de init de SourceTVSupport no cambian y los
   pipes cliente/gameserver siguen siendo 1/2. Observar otros tres segundos ausente.
4. Recargar la extensión por consola y el consumidor HTTP. Biblioteca/convar,
   hooks y HTTP vuelven a funcionar. Repetir descarga/recarga una segunda vez.
5. Con la tercera carga activa, ejecutar `map c2m2_fairgrounds`: se observa init
   original exitoso y HTTP vuelve a pasar después del nuevo juego.
6. Terminar mediante quit controlado y LinuxGSM; restaurar el DEV habitual.

| Comprobación | Resultado |
|---|---|
| Proceso | Mismo PID **105960** y ticks de inicio durante carga, dos ciclos y nuevo juego |
| Biblioteca | Tres eventos Added y dos Removed, sin duplicación de altas/bajas |
| Descarga | Dos desmapeos reales; biblioteca/convar retiradas y hooks restaurados |
| Propiedad Steam | Pipes cliente/gameserver 1/2 preservados durante ambas descargas/recargas |
| SourceTVSupport | Entrada init inspeccionada sin cambios durante los ciclos |
| HTTP | Cuatro rondas; una completion por ronda, cuerpo de 16 bytes y ambas celdas del contexto correctos, handle cerrado |
| Salida | Quit limpio, sin errores SourceMod, marcadores de crash ni nuevo core del candidato |

Las tres fuentes de harness compilan sin warnings. El único archivo nuevo es
`tests/runtime/steamworks_extension_observer.sp`; la extensión, API pública, VM y
bibliotecas del juego no se modificaron. No se repitieron autenticación o stats.

## Restauración y límites

LinuxGSM arrancó el DEV habitual, PID **108339**, con selector y extensión
originales, Steam conectado y cuatro hashes protegidos correctos. Los plugins
originales nunca se movieron fuera del servidor habitual: solo se retiraron sus
copias dentro del laboratorio. Se eliminaron laboratorio/recovery y fixture.
Los cores originales de systemd se conservan; no se exportan cores/argv/entorno.

Esta aceptación no certifica unload desde una pila de callback activa, descarga
forzada de consumidores dependientes, solicitudes activas al retirar la extensión,
toda la pila de plugins, ni ausencia global de fugas. El contrato probado es la
retirada serial por consola después de cerrar peticiones y retirar consumidores.
Los pendientes T11/T15/T23/T25/T29/T32/T33 siguen abiertos; P5 no tiene aceptación global.

## Evidencia

[Resultados/hashes](t16-extension-srcds/results.json),
[host frío sin SteamWorks](t16-extension-srcds/cold-absent.json),
[carga tardía](t16-extension-srcds/late-load.json),
[descarga 1](t16-extension-srcds/unloaded-1.json),
[descarga 2](t16-extension-srcds/unloaded-2.json),
[segunda recarga y HTTP](t16-extension-srcds/reloaded-http-2.json),
[nuevo juego](t16-extension-srcds/newgame-after-reloads.json),
[HTTP posterior](t16-extension-srcds/post-newgame-http.json),
[salida](t16-extension-srcds/shutdown.json),
[cores](t16-extension-srcds/core-audit.json) y
[restauración final](t16-extension-srcds/cleanup.json).

Paquete separado:
`.build/t16-extension-srcds/review/steamworks-t16-extension-linux-x86-srcds-acceptance.zip`,
con manifiesto/checksums, evidencia filtrada y fuentes de harness/runner. Sin CFG
privados, SMX de terceros, SDK, módulos del juego, logs crudos ni cores.
Los ZIP P5-R11 permanecen intactos; la
[matriz anterior](p5/history/p5-r11-before-T16-extension-acceptance-validation-results.json)
se conserva y la matriz activa registra el nuevo harness posterior al empaquetado.
