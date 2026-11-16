# P5-R2: reintento con jugador real

Seguimiento: la [revisión de BeginAuthSession](auth-debug.md) añade desensamblado,
probes de los objetos 014/015 y una convar diagnóstica todavía no instalada.
Este informe conserva la evidencia del candidato P5-R2 original.

Fecha local: 2026-11-14, America/Santiago. Rama `develop`, sin commit ni
intervención en producción. Este reintento complementa la
[primera aceptación](srcds-acceptance.md), cuyos archivos se conservan.
El candidato no cambia: SHA-256
`4830b71afa8d1a8b64e412071871165f849953667cc89499bd04612a1589687e`.

**El acceso, la identidad y los stats enteros con jugador real pasan. La
aceptación global sigue sin aprobarse**, por el contexto HTTP streaming registrado
anteriormente y porque no se observó el forward interno `BeginAuthSession`.
Los forwards públicos de validación sí se recibieron durante la reconexión.

## Entorno reproducido

Primero se confirmó una cuenta conectada al DEV original. Para ejecutar el
candidato se detuvo ese servicio mediante LinuxGSM y se seleccionó un árbol
`addons/sourcemod_acceptance`. El CFG LinuxGSM solo cambió en `basepath`; se
conservaron sus parámetros de arranque, el CFG de juego, puertos y SourceTV.
No se asignó contraseña ni se ejecutaron setters de metadatos del harness.

El árbol mínimo volvió a fallar al enlazar 27115. Al copiar al árbol aislado
los plugins, extensiones y configuraciones completos del DEV, junto con una
copia de sus datos, el servidor arrancó, conectó a Steam y admitió al jugador.
Los archivos originales de SourceMod no se reemplazaron. Se desactivó el
actualizador de SourceMod en la copia; la primera edición de ese valor tenía
comillas escapadas incorrectamente y se corrigió. No se atribuye el cambio de
resultado a una dependencia concreta: no se hizo una eliminación individual de
plugins/extensiones ni se aisló la causa del fallo del árbol mínimo.

El candidato exacto se verificó entre los módulos realmente mapeados, antes y
después de la reconexión. Se mantuvo la VM instalada; no es una ejecución ASan
de SRCDS. Las configs privadas copiadas se retiraron con el laboratorio y no se
incorporan al paquete de evidencia.

## Comprobaciones con la cuenta del operador

| Comprobación | Resultado observado |
|---|---|
| Acceso autenticado al candidato | Un humano autorizado dentro del juego |
| `GetClientSteamID` | Coincide con SteamID64 de SourceMod, conteo incluye NUL, buffer pequeño termina, capacidad cero conserva destino |
| Licencia AppID 550 | `HasLicenseForApp` devuelve 0, `HasLicense` |
| `RequestStats(client, 550)` | Aceptada |
| `RequestStatsAuthID(account, 550)` | Aceptada |
| `GetStatAuthIDCell` | Lectura exitosa de `Stat.GamesPlayed.Total` |
| `GetStatCell` | Lectura exitosa y valor igual al obtenido por AccountID |
| Leer esa clave integer como float | Falla y conserva el sentinel, tanto por cliente como por AccountID |
| `SteamWorks_OnValidateClient` | Un evento durante la segunda entrada |
| `SW_OnValidateClient` | Un evento durante la segunda entrada |
| `SteamWorks_BeginAuthSession` | Cero eventos observados; transporte real de T36 no aceptado |

La traza final incluye:

```text
[SW-ACCEPT] client_identity pass=1 clientStats=1 authStats=1 license=0
[SW-ACCEPT] authorized_humans=1
[SW-ACCEPT] validation_events current=1 legacy=1
[SW-ACCEPT] authorized_stats_read integer=1 float=0 intFailurePreserved=1 floatFailurePreserved=1
[SW-ACCEPT] client_stats_read integer=1 matchesAccount=1 float=0 floatFailurePreserved=1
```

El harness no lee ni imprime tickets, identidades o valores de stats. Los eventos
públicos solo se cuentan; esta prueba no certifica los argumentos owner/child ni
un caso Family Sharing. Un contador global no equivale a probar toda la semántica
del forward.

Al principio el harness consultó un cliente durante su transición y SourceMod
rechazó el índice. Se corrigió el harness para exigir `IsClientInGame` y
revalidar cuenta/estado antes de las lecturas diferidas por índice. Se recompiló
sin warnings. La extensión y su API no se modificaron. Se añadieron contadores
para ambos forwards públicos y una comparación de stats por cliente/AccountID.

La primera reconexión coincidió con un cierre de proceso y el reinicio automático
de LinuxGSM. Se observó shutdown del runtime y una nueva conexión Steam. No se
ha determinado qué solicitó ese reinicio; no se cuenta como prueba de estabilidad
ni se oculta como un simple cambio de mapa. Tras el siguiente intento del
operador se observaron los dos eventos públicos y las lecturas positivas finales.

Como pista estática, engine y libsteam_api contienen el literal
`SteamGameServer014`, mientras el contrato tipado de la extensión solicita
`SteamGameServer015`. La presencia de esos literales no demuestra la ruta de
llamada ni la causa de los cero eventos del hook. No se aceptó 014 mediante un
cast a 015 ni se añadió una versión desconocida para forzar el resultado.

## Cierre

Terminadas las lecturas se detuvo el laboratorio, se restauró byte a byte el CFG
original y se inició el DEV habitual con LinuxGSM. La evidencia de cierre verifica
el selector `addons/sourcemod1`, el binario anterior realmente mapeado, la conexión
Steam y los hashes originales del CFG y las configs comprobadas de consumidores.
Se retiró el árbol aislado, incluidos los datos y configs privados copiados.

Los [resultados y hashes](srcds-acceptance-retry/results.json) identifican trazas,
módulos, build del harness y restauración. El archivo de evidencia independiente
está en `.build/srcds-acceptance-retry/review/`; los ZIP originales P5-R2 y los del
primer ensayo permanecen sin sobrescribir. Siguen pendientes streaming, el hook
interno de autenticación, stat float positivo, GC, semántica completa de consumidores,
la causa del reinicio y operación prolongada.
