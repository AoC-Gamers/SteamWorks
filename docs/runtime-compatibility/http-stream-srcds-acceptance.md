# Aceptación del contexto HTTP streaming en SRCDS x86

Fecha local: 2026-11-14, America/Santiago. Rama `develop`, sin commit ni producción.
**Pasa la aceptación acotada del contexto HTTP streaming** en `competitive_dev`,
LinuxGSM, puerto 27015 y un árbol SourceMod aislado. El binario exacto tiene
SHA-256 `bc76b00d0614f8f8489f8d3d30e3d654976f3bfde689f17d8731018d6c7ac639`.
El DEV habitual quedó restaurado y conectado a Steam al finalizar.

## Candidato y aislamiento

La [corrección local](http-stream-context.md) conserva el contexto completo por
petición tras un setter aceptado. Se verificaron sus hashes de código, include y
binario contra el resultado local anterior: 92 comprobaciones deterministas x86,
control negativo, sanitizadores y 11 suites integradas con comparación de API P0.
No se modificó código de la extensión durante esta aceptación.

Se detuvo temporalmente el DEV habitual mediante `/app/l4d2server stop` y se
arrancó el laboratorio con `/app/l4d2server start`, bajo el entorno protegido de
LinuxGSM y el usuario `linuxgsm`. El único cambio del CFG LinuxGSM fue seleccionar
`addons/sourcemod_acceptance` en lugar de `addons/sourcemod1`. Se conservaron
puertos, parámetros, SourceTV, contraseña y configuraciones habituales de juego.

El árbol aislado copió las extensiones/plugins/configs y datos necesarios;
solamente allí se sustituyeron SteamWorks y su include y se añadió el harness.
Se desactivó el actualizador SourceMod de esa copia. No se reemplazaron Steam,
SourcePawn ni bibliotecas del motor. La fixture HTTP/TLS escuchó exclusivamente
en loopback; el certificado era autofirmado y la verificación TLS permaneció
activada para probar su rechazo. No fue necesaria una cuenta ni autenticación real.

Se comprobaron dispositivo/inodo y hash de diez módulos realmente mapeados,
incluidos engine, libsteam_api, ambos steamclient, tier0, VM y la extensión.
El candidato ELF32/i386 y el mismo PID permanecieron cargados después del cambio
de mapa. El perfil fue `l4d2-23990100-linux-x86`; el debug se activó con máscara 12.

## Resultado

Se compiló el harness original, sin cambiar sus comprobaciones, con SourcePawn
1.12.0.7259 y sin warnings. Cada tanda `sm_sw_accept http` encola ocho casos:
texto, binario con NUL, vacío, HTTP 503, timeout, TLS inválido, conexión rechazada
y streaming. El contexto del streaming es `(7, 0x12345678)` y se comprueban ambas
celdas en headers/data, además de los seis bytes del payload.

| Fase | Nuevos completions | Headers | Callbacks data | Bytes streaming | Fallos |
|---|---:|---:|---:|---:|---:|
| Primera tanda | 8 | 1 | 2 | 6 | 0 |
| Dos tandas consecutivas | 16 | 2 | 4 | 12 | 0 |
| Tras `changelevel c1m1_hotel` | 8 | 1 | 2 | 6 | 0 |
| Total | 32 | 4 | 8 | 24 | 0 |

En las dos tandas consecutivas, ambas peticiones de streaming quedaron encoladas
antes del primer callback de headers. No se exige ni se afirma que sus fragmentos
se intercalen: se valida el aislamiento de peticiones pendientes y sus callbacks.

La primera tanda produjo el resultado que fallaba en P5-R2:

```text
[SW-ACCEPT] headers case=7 failure=0 context=1
[SW-ACCEPT] data case=7 payload=1 context=1 offset=0 bytes=3
[SW-ACCEPT] data case=7 payload=1 context=1 offset=3 bytes=3
[SW-ACCEPT] complete case=7 pass=1 failure=0 success=1 status=200 sizeOk=1 size=0 timeoutOk=1 timeout=0
[SW-ACCEPT] status loaded=1 connected=1 vac=1 ipv4=1 completed=8 failed=0 headers=1 streamBytes=6 authEvents=0
```

Tras el cambio de mapa, los contadores acumulados fueron `completed=32 failed=0
headers=4 streamBytes=24`. También se envió y cerró una petición `/delay`:
`cancel_sent=1`, sin callback posterior durante la espera de siete segundos.
La fixture recibió cuatro solicitudes por endpoint y cinco `/delay`, incluida
la cancelada. No se crearon logs de error SourceMod ni se observaron marcadores
de crash en las fases registradas.

## Cierre y reversión ejecutada

El cierre por consola `quit` registró `gameserver_detach generation=1`,
`Steam lifecycle shutdown observed: consumers stopped` y
`plugin_end completed=32 failed=0`. Se detuvo después el wrapper LinuxGSM y se
restauró byte a byte el CFG original, SHA-256 `ca1b5179…`.

El DEV habitual volvió a arrancar con LinuxGSM, `addons/sourcemod1`, la extensión
original mapeada (`9ca37aad…`) y conexión a Steam. Se verificaron también los
hashes originales de los CFG de AoC y Family Share. Se retiraron el árbol aislado,
sus copias privadas, la fixture y el certificado/clave temporales. No se cambiaron
archivos de producción ni se conservan credenciales en la evidencia publicada.

## Evidencia y límites

[Resultados y hashes](http-stream-srcds/results.json), trazas por fase,
[módulos cargados](http-stream-srcds/loaded-modules-final.json),
[cierre](http-stream-srcds/shutdown.json),
[restauración](http-stream-srcds/cleanup.json),
[conteos HTTP](http-stream-srcds/fixture-counts.json) y
[harness exacto](http-stream-srcds/harness.sp) se guardan en `http-stream-srcds/`.
Los artefactos locales de revisión están en `.build/http-stream-srcds/review/`.
Los informes y ZIP previos de P5-R2, R3 y validación local permanecen sin sobrescribir.

Esta aceptación cierra el fallo de contexto streaming para este binario/perfil y
los escenarios descritos. No acepta P5 globalmente, GC, todos los consumidores,
operación prolongada, descarga en caliente ni autenticación de un jugador con
este nuevo binario. SRCDS usó la VM instalada, sin ASan; los sanitizadores
corresponden a las pruebas deterministas locales anteriores. La API pública se
conserva y todos los cambios siguen sin commit.
