# P5-R3: aceptación de autenticación, identidad y stats en SRCDS x86

**Pasa la aceptación acotada con un jugador autorizado en el candidato final**,
SHA-256 `bc76b00d0614f8f8489f8d3d30e3d654976f3bfde689f17d8731018d6c7ac639`.
Se usó `competitive_dev`, LinuxGSM y un árbol SourceMod aislado en 27015. Rama
`develop`, sin commit ni producción. El DEV habitual ya está restaurado.

## Paquete y entorno comprobados

La extensión y el include se extrajeron del ZIP de candidato P5-R3, comprobando
su checksum y los hashes del manifiesto. No se sustituyó el payload por otro build.
El harness original se recompiló con SourcePawn 1.12.0.7259, sin warnings.

LinuxGSM detuvo temporalmente el DEV habitual y arrancó el laboratorio. Solo se
cambió el selector `basepath` a `addons/sourcemod_acceptance`; se conservaron los
parámetros, puertos, contraseña y CFG habituales de juego/SourceTV. No se añadió
contraseña ni se modificaron metadatos para permitir la entrada. La copia aislada
conservó plugins/configs/datos necesarios y desactivó su actualizador SourceMod.
No se reemplazaron la VM instalada, Steam ni bibliotecas del motor.

Los módulos realmente mapeados se identificaron por dispositivo/inodo y hash,
antes y después de las pruebas, con el mismo PID y candidato ELF32/i386. El perfil
fue `l4d2-23990100-linux-x86`. Se activó `steamworks_debug 12` antes de la conexión;
el estado confirmó interfaz nativa 015 y destino de autenticación 014 separados.

## Resultados con la cuenta del operador

| Comprobación | Resultado observado |
|---|---|
| Entrada nueva | Un humano autorizado y dentro del juego |
| BeginAuthSession | Un evento recibido por el harness; despacho SourceMod con resultado 0 |
| Validación Steam | `ValidateAuthTicketResponse result=0` |
| Forwards públicos | Un evento en el forward actual y otro en el legacy |
| Identidad | SteamID textual igual al obtenido por SourceMod |
| Conteo/buffers | Conteo con NUL, buffer pequeño terminado y capacidad cero sin escritura |
| Licencia AppID 550 | `HasLicense`, valor 0 |
| RequestStats | Aceptado por índice de cliente y por AccountID |
| Stat entero | `Stat.GamesPlayed.Total` leído por ambas rutas, valores iguales |
| Esa clave entera como float | Fallo esperado; sentinels conservados por ambas rutas |

Traza filtrada del harness:

```text
[SW-ACCEPT] client_identity pass=1 clientStats=1 authStats=1 license=0
[SW-ACCEPT] authorized_humans=1
[SW-ACCEPT] authorized_stats_read integer=1 float=0 intFailurePreserved=1 floatFailurePreserved=1
[SW-ACCEPT] client_stats_read integer=1 matchesAccount=1 float=0 floatFailurePreserved=1
[SW-ACCEPT] status loaded=1 connected=1 vac=1 ipv4=1 completed=0 failed=0 headers=0 streamBytes=0 authEvents=1
[SW-ACCEPT] validation_events current=1 legacy=1
```

El debug observó `begin_auth_enter`, `begin_auth_dispatch result=0`, validación
Steam exitosa y despacho de ambos forwards públicos. El forward actual tenía
otros receptores en el stack: `receivers=3` no significa tres eventos del harness.
La recepción y la validación Steam se comprueban por separado; el resultado 0 del
despacho es el código SourceMod, no el retorno de BeginAuthSession de Steam.

No se inspeccionaron ni persistieron tickets o AccountIDs del forward. Los datos
binarios del contrato conservan sus pruebas deterministas con CForward/VM; no se
compararon bytes de un ticket real. Tampoco se escribieron valores de stats.
No se crearon logs de errores SourceMod ni se observaron marcadores de crash.

## Cierre y restauración

El cierre explícito `quit` registró `gameserver_detach generation=1`, consumidores
detenidos y `plugin_end completed=0 failed=0`. Esos contadores HTTP en cero son
propios de esta sesión de autenticación; no reemplazan la aceptación HTTP anterior.
Se detuvo el wrapper LinuxGSM y se restauró byte a byte el CFG original.

El DEV habitual volvió con `addons/sourcemod1`, extensión original mapeada
(`9ca37aad…`) y conexión a Steam. Coinciden los hashes originales del selector
(`ca1b5179…`), extensión y CFG de AoC/Family Share. Se retiró todo el árbol aislado,
incluidas sus copias privadas. Producción permanece intacta.

## Evidencia y alcance

[Resultados/hashes](p5-r3-auth-srcds/results.json),
[comprobaciones](p5-r3-auth-srcds/checks.json),
[traza filtrada](p5-r3-auth-srcds/runtime.json),
[módulos cargados](p5-r3-auth-srcds/loaded-modules-final.json),
[cierre](p5-r3-auth-srcds/shutdown.json) y
[restauración](p5-r3-auth-srcds/cleanup.json) conservan esta ejecución.
La evidencia de revisión se guarda en `.build/p5-r3-auth-srcds/review/`.

Esto cierra la entrada real, identidad y stats enteros del binario final que
seguían pendientes tras el R3 anterior. T36 incluye ahora recepción real con el
candidato final; T26 registra su payload instalado desde el ZIP revisado. La
[matriz P5](p5/validation-results.json) distingue los casos aún parciales.

No aprueba P5 globalmente ni la matriz completa de stats/forwards/consumidores.
Quedan abiertos, entre otros, una clave float positiva y claves inexistentes,
negativos de identidad en host real, GC, operación sostenida, ciclo de vida
restante y restauración en sitio desde el ZIP de reversión. Los ZIP P5-R3 previos
se conservan sin regenerar; su documentación es el checkpoint previo a este
seguimiento. SRCDS no estuvo instrumentado con ASan.
