# Aceptación SRCDS de P5-R2

Actualización: el [reintento con jugador real](srcds-acceptance-retry.md) valida
acceso, identidad, stats enteros por cliente y AccountID y ambos forwards públicos
de validación. Este informe conserva la primera ejecución; sus pendientes se
reconcilian en el informe posterior. La aceptación global continúa sin aprobarse.

El [seguimiento del contexto HTTP streaming](http-stream-context.md) reproduce su
causa fuera de SourceMod y registra una corrección local con pruebas x86. La
[aceptación posterior de esa corrección](http-stream-srcds-acceptance.md) pasa en
SRCDS con un nuevo binario; los resultados históricos siguientes no cambian.

Fecha: 2026-11-14. Rama `develop`, base
`46e3ee05fc29ad090a1d6f7b1cb5e3dca9855350`; cambios sin commit.
**Resultado global: no aceptado.** Se observaron avances de integración real y
un fallo reproducible de contexto HTTP streaming. Identidad y autenticación de
un jugador en el candidato siguen pendientes. No se modificó el candidato para
ocultar el resultado ni se desplegó en producción.

La [evidencia estructurada](srcds-acceptance/results.json) y las trazas filtradas
complementan el [checkpoint local P5-R2](p5-integration.md). Sus ZIP y matriz
originales permanecen intactos como captura anterior a esta intervención.

## Entorno y arranque

Se usó `competitive_dev`, con red host y un árbol SourceMod aislado
`addons/sourcemod_acceptance`; no se sustituyó la VM instalada
(SourceMod 1.12.0.7230). El candidato cargado tiene SHA-256
`4830b71afa8d1a8b64e412071871165f849953667cc89499bd04612a1589687e`.
La identidad de los archivos mapeados se contrastó con dispositivo/inodo y hashes.
El perfil es `l4d2-23990100-linux-x86`; engine y libsteam_api corresponden a P4.
Esto no amplía el registro a otras parejas ni certifica todo SRCDS bajo ASan.

Los primeros ensayos usaron lanzamiento directo y puerto 27185. A petición del
operador se pasó a 27015 y después al supervisor LinuxGSM de `/app/l4d2server`.
Se conservó una copia privada del CFG y se seleccionaron temporalmente el árbol
SourceMod y `steamworks_acceptance.cfg`. El CFG aislado usaba `sv_lan 0`,
`sv_allow_lobby_connect_only 0` y `sv_hibernate_when_empty 0`.

El primer intento con contraseña se descartó tras la indicación del operador;
el arranque LinuxGSM final no asignaba contraseña. El autoexec compartido activaba
SourceTV y se observó un fallo de bind en 27115. Desactivar TV en el CFG tardío
no bastó: se antepuso `+tv_enable 0` al template original de LinuxGSM, sin cambiar
sus demás parámetros. Tanto candidato como baseline conectaron a Steam tras ese
ajuste. No se atribuye el fallo inicial exclusivamente a SteamWorks.

El acceso de un jugador al laboratorio no quedó confirmado. La consulta A2S local
expiró pese a que la consola mostraba conexión Steam y VAC activo. No se ha
identificado la causa del acceso fallido: conectar a Steam no demuestra que el
cliente pueda entrar. Los comandos completos, contraseñas, identidades, tickets
y claves privadas no forman parte de la evidencia publicada.

## Resultados observados

| Caso | Resultado y alcance |
|---|---|
| Arranque del candidato | Cargado, conectado a Steam, VAC e IPv4 disponibles con LinuxGSM |
| T16: mapa y recarga | Cambio de c2m1_highway a c1m1_hotel; descarga/recarga explícita de extensión y harness, con estado cargado/conectado posterior |
| T17: HTTP texto/binario | Texto correcto y seis bytes `A,0,B,255,0,Z`; cuerpo vacío y terminación textual correctos |
| T18: errores | HTTP 503 conservado como respuesta HTTP; conexión rechazada, timeout con flag y certificado autofirmado rechazado con verificación activa |
| HTTP streaming | Los dos fragmentos de tres bytes son correctos; contexto incorrecto en headers/data. El completion conserva contexto y devuelve HTTP 200. **Falla** |
| Stats negativos | Stat inexistente conserva sentinelas int/float; AppID incorrecto e identidad cero se rechazan |
| Stats de cuenta autorizada | Solicitud aceptada y lectura integer de `Stat.GamesPlayed.Total` exitosa; leer esa clave como float falla y conserva el sentinel. No certifica un stat float positivo |
| Identidad de cliente/T36 real | Cero humanos autorizados y cero eventos de autenticación en el laboratorio; pendientes |
| T20: propiedad HTTP | Cierre del handle dentro del completion; cancelación inmediata; descarga del plugin dueño antes de respuesta sin callback posterior observado |
| Consumidores | AoC recompilado y Family Share precompilado/recompilado cargan y descargan; no se certifican sus flujos completos con jugadores |
| Recursos | 80 peticiones adicionales en 80,52 s; RSS +12 KiB y VmSize estable. Cero HTTPHandle al inicio y al final; no es una prueba prolongada ni demuestra ausencia global de fugas |
| Reversión | Binario e include anteriores extraídos del ZIP, gamedata ausente, arranque LinuxGSM y conexión Steam verificados en el laboratorio |
| Restauración de DEV | CFG original restaurado byte a byte; proceso con `addons/sourcemod1`, extensión anterior mapeada y conectado a Steam; laboratorio retirado |

Los ensayos directos iniciales y los de LinuxGSM se conservan separados. La
lectura positiva por AccountID y las 80 peticiones pertenecen a la fase directa;
los casos HTTP y las pruebas finales de consumidores/descarga también se
repitieron con LinuxGSM. No se heredan a este candidato las pruebas de P4.

## Fallo de streaming

El harness configura contexto `(7, 0x12345678)` y comprueba ambas celdas. En el
arranque LinuxGSM final se registró:

```text
[SW-ACCEPT] headers case=0 failure=0 context=0
[SW-ACCEPT] data case=0 payload=1 context=0 offset=0 bytes=3
[SW-ACCEPT] data case=0 payload=1 context=0 offset=3 bytes=3
[SW-ACCEPT] complete case=7 pass=1 failure=0 success=1 status=200 sizeOk=1 size=0 timeoutOk=1 timeout=0
[SW-ACCEPT] status loaded=1 connected=1 vac=1 ipv4=1 completed=8 failed=3 headers=1 streamBytes=0 authEvents=0
```

`context=0` es el booleano de comparación, no el valor bruto de las dos celdas.
`case=0` muestra que la primera celda no conserva el 7 solicitado. El contador
`streamBytes=0` solo suma bytes cuando payload y contexto pasan juntos; no implica
que Steam no haya entregado datos. La comprobación de headers se reforzó para
validar ambas celdas y se recompiló y repitió antes de cerrar la evidencia.
No se ha aislado todavía si la discrepancia nace en el proveedor Steam, el ABI o
el transporte de la extensión; resolverla requiere una investigación posterior.

Hubo dos errores administrativos corregidos: un unload inicial usó un nombre de
plugin incorrecto, y un comando de consola demasiado largo truncó el nombre de
un consumidor. Esas ejecuciones no cuentan como pruebas aprobadas. Las repeticiones
con nombres completos y comandos separados sí aparecen en la traza. AoC necesitó
las extensiones estándar SDKHooks/SDKTools/BinTools en el árbol aislado. Algunos
cvars de plugins ausentes generaron avisos; las configs compartidas no cambiaron.

## Cierre y pendientes

Se restauró el CFG LinuxGSM original, SHA-256
`ca1b5179fd3ea4154f3a2d8de41f1e6eaea0f93ba6598b2047498b371802bd00`.
La extensión original cargada conserva SHA-256
`9ca37aadcd83448d49126d6a72babcb8ec97ec5a06c171b4821ae0ee3cf05d70`.
Se verificaron también los hashes previos de AoC.cfg y Family Share. Se retiraron
el árbol, el CFG de laboratorio, el volcado auxiliar y el fixture con sus claves.
El servicio habitual quedó iniciado por LinuxGSM y conectado a Steam.

Quedan abiertos: contexto streaming, acceso/autenticación de un humano, identidad
y stats por índice de cliente, stat float positivo, semántica completa de los
consumidores/forwards, GC remoto, desconexión controlada y prueba prolongada.
La API pública y el payload candidato se conservan. Los nuevos harnesses tienen
[instrucciones propias](../../tests/runtime/README.md); no se incluyen en el ZIP
instalable ni habilitan automáticamente una intervención posterior.
