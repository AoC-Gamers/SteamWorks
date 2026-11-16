# P5-R6: autenticación, identidad y stats aceptados en SRCDS x86

**Pasa la aceptación acotada con una cuenta autorizada** para el ELF
`82347b99d5130bd411cdc0b35d215dc87fc82d394a2252ccd83d5d4940b6ca5a`.
Se extrajeron extensión/include del ZIP P5-R6 revisado, contrastando su checksum
y todos los hashes del manifiesto. No se sustituyó el payload por otro build.

LinuxGSM inició el árbol SourceMod aislado de `competitive_dev` en 27015, con
SourceTVSupport y las configuraciones/flags habituales; solo cambió el selector
`basepath`. No se añadió contraseña ni se reemplazaron VM, SDK o módulos del juego.
Los dos harnesses se recompilaron sin warnings con SourcePawn 1.12.0.7259 y el
include del ZIP; el fixture conserva las 733 claves públicas del schema AppID 550.

## Autenticación e identidad

Con `steamworks_debug 12` activo antes de la entrada real, se observaron:

| Comprobación | Resultado |
|---|---|
| Humano autorizado dentro del juego | Uno |
| BeginAuthSession recibido por el harness | Un evento |
| Despacho SourceMod del forward | Código 0 |
| ValidateAuthTicketResponse de Steam | Resultado 0 |
| Forwards públicos actual y legacy | Un evento en cada uno |
| Destinos tipados separados | Natives 015, autenticación 014 |
| SteamID textual y SourceMod | Iguales |
| Conteo y buffers | NUL incluido, buffer pequeño terminado, capacidad cero sin escritura |
| Licencia AppID 550 | `HasLicense`, código 0 |

El código 0 del despacho es el resultado SourceMod, no el retorno de la función
Steam BeginAuthSession. La validación Steam se acredita por su callback separado.
No se inspeccionaron ni guardaron tickets o AccountIDs del forward. Los datos
binarios del marshalling siguen acreditados por las pruebas deterministas.

## Stats de solo lectura

`RequestStats` se aceptó por cliente y AccountID. `Stat.GamesPlayed.Total` se leyó
como entero por ambas rutas con valores iguales; consultarla como float falló
conservando ambos sentinels. No se registraron valores personales de stats.

Después se realizaron dos recorridos completos:

| Comprobación | Ronda 1 | Ronda 2 |
|---|---:|---:|
| Claves recorridas | 733 | 733 |
| Float exitoso por ambas rutas | 12 | 12 |
| Discrepancias, valores no finitos o salidas alteradas al fallar | 0 | 0 |
| Clave inexistente, antes y después | 2 | 2 |

Las mismas 12 claves entregaron floats finitos e idénticos en bits por cliente
y AccountID. «Exitoso» no exige un valor mayor que cero. Las otras 721 claves
rechazaron float conservando los sentinels; no se infiere que todas sean enteras
o inexistentes.

La clave negativa se leyó como entero y float por ambas rutas: **16 lecturas**
individuales entre ambas rondas, todas rechazadas y con las cuatro salidas
preservadas, incluidos los bits exactos de dos sentinels NaN distintos. Cada
ronda esperó un entero válido para evitar aprobar fallos con un cache sin cargar.
Los timers revalidaron userid/AccountID; no se escribieron stats.

## Cierre y restauración

Dispositivo/inodo y hash de los módulos cargados se comprobaron antes/después,
con el mismo PID **65649** y start ticks. No hubo errores SourceMod. El `quit`
final completó la parada sin SIGSEGV ni nuevos cores desde la captura inicial.

Se restauró el CFG original y LinuxGSM volvió a iniciar el DEV habitual. Selector,
extensión mapeada y los cuatro hashes protegidos coinciden con el baseline;
Steam quedó conectado y el laboratorio se retiró. La restauración del selector
no equivale a aplicar el ZIP de rollback R6 en sitio, que sigue pendiente.

[Resultados/hashes](p5-r6-auth-srcds/results.json),
[autenticación e identidad](p5-r6-auth-srcds/checks-auth.json),
[primera ronda](p5-r6-auth-srcds/stats-round-1.json),
[segunda ronda](p5-r6-auth-srcds/stats-round-2.json),
[módulos finales](p5-r6-auth-srcds/loaded-modules-final.json),
[cierre](p5-r6-auth-srcds/shutdown.json) y
[restauración](p5-r6-auth-srcds/cleanup.json). Evidencia local de revisión en
`.build/p5-r6-auth-srcds/review/`, sin configs privadas, comandos con credenciales,
módulos propietarios ni cores.

## Alcance

La [matriz P5](p5/validation-results.json) incorpora resultados directos sobre
este ELF, además de su [aceptación T08 con SourceTVSupport](t08-coexist-srcds-acceptance.md).
T01–T03 cierran el pendiente humano de stats; T26 acredita el payload del ZIP R6
instalado y T36 añade recepción real. Identidad y forwards conservan sus negativos
y matrices semánticas todavía abiertos; tampoco se aprueban GC, hot unload,
desconexión de Steam, carreras adicionales u operación sostenida.

P5 sigue sin aceptación global. La API pública permanece intacta, rama `develop`,
sin commit ni producción. Los ZIP y la verificación de preparación P5-R6 son el
checkpoint previo y no se sobrescriben para atribuirles esta aceptación posterior.
