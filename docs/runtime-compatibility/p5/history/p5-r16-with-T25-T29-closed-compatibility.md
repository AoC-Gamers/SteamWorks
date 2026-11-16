# Registro de compatibilidad

## Estado tras la reanudación T25/T29

[T25/T29 aceptados](last-two-srcds-progress.md): préstamo, perfil y grupo reales;
cliente pendiente→autorizado; Connect corregido y verificado desde arranque frío,
HTTP/ACK y cambio de mapa. Quince suites x86, 104 checks Connect y 66 pruebas Python
pasan. La matriz Linux x86 cierra 33 casos con tres no aplicables; no certifica
negocio completo, otros proveedores ni plataformas. ZIP R16/ELF/include intactos,
parche externo y evidencia separados. DEV restaurado, sin commit ni producción.

## Registro histórico anterior a este cierre

Las referencias a casos parciales y operaciones de empaquetado siguientes
describen sus checkpoints originales; el estado vigente es el indicado arriba.


P5-R16 es candidato local de revisión, sin publicación ni aceptación operativa global.
SDK objetivo 1.58a; no se cambió el SDK del juego.

| Plataforma/módulos | Ruta interna | Validación |
|---|---|---|
| Linux i386, L4D2 build 23990100, pareja de P4 | `l4d2-23990100-linux-x86` | Optimizado `839ff075…`: dos reinits con SourceTVSupport, HTTP/streaming, auth/identidad/stats humanos y quit limpio aceptados; T26/T35 del ZIP R9 pasan en su alcance |
| Otra pareja Linux, aunque comparta nombre/versión visible | Rechazada | Requiere evidencia y perfil nuevo |
| Linux x86_64 | Rechazada | Solo historia diagnóstica; sin nuevos builds ni artefactos |
| Windows | Rechazada | Fuera del candidato; herramientas generales no certifican ABI |

Identidades/prefijos: [P4](p4-binary-profiles.md) y
[perfil documental](p4/competitive-dev-linux-x86.json). Los JSON no habilitan rutas
del producto. No se distribuyen bibliotecas Steam/engine/SDK.

## Compatibilidad de consumidores

H08 conserva ambas firmas de solicitud de stats. El segundo argumento es el
AppID del contexto, no el universo Steam. Un plugin que pasaba `1` como universo
debe usar el AppID correspondiente (550 en L4D2). El primer argumento AuthID es
AccountID de 32 bits, no SteamID64 truncado. Una solicitud aceptada sigue siendo
asíncrona. [Contrato y pruebas](p1-b-stats-identity.md).

H09 conserva el resultado histórico de `GetClientSteamID`: para capacidad
positiva, el conteo incluye NUL. Cero devuelve cero sin escribir; tamaños negativos
son error. Los consumidores no deben sumar otro terminador al conteo devuelto.
[Conciliación y consumidores](p1-e-steamid-count.md).

El helper HTTP textual exige sus cinco argumentos existentes. Dos llamadas de
Connect (`connect/http.inc`, `connect/ingest.inc`) siguen pasando tres argumentos.
El native las rechaza con error controlado antes de leer salidas ausentes. Ese
consumidor requiere corrección externa antes de incluirlo en la aceptación.
No se cambia su repositorio desde este trabajo.

P5-R16 recompila el ejemplo, AoC y Family Share sin warnings y comprueba cuatro
tablas de imports, una precompilada. El inventario examina 2697 archivos y encuentra
40 con referencias. El corpus léxico y la carga de 138 plugins en el ensayo
optimizado no acreditan toda la semántica de consumidores. El seguimiento
[T25](consumer-compat-srcds-acceptance.md) acepta las rutas acotadas de seis variantes
reales, 37 checks, y 22 checks H08/H09 en dos controles nuevos compilados contra
includes antiguo/actual. T25 queda parcial por Connect y flujos completos pendientes.

Se conservan dos observaciones de P0: faltan marcaciones opcionales para
`SetMapName` y `GetHTTPRequestWasTimedOut`, y existe el forward interno no declarado
`SteamWorks_BeginAuthSession`. [T36](t36-auth-forward.md) conserva `Param_Array`
y sus tres argumentos, con transporte compacto por `PushArray`, CForward/VM
reales y límites de reserva probados bajo optimización. El
[adaptador 014](begin-auth-session-014.md) mantiene un tipo separado de la interfaz
nativa 015. La API pública conserva firmas, registros, valores y opcionalidad.

## Atribución de evidencia

| Binario/checkpoint | Evidencia y límite |
|---|---|
| Optimizado `839ff075…`, P5-R9/R10/R11/R12/R13/R14/R15/R16 | [SRCDS directo](optimized-srcds-acceptance.md): dos reinits, 25 HTTP/18 bytes streaming, petición retenida, un humano con auth/identidad/stats y quit limpio; build R9 idéntico. T26/T35 con ZIP real aceptados en su seguimiento |
| Canónico `75a9b5d9…` | Build local pasa; no se ejecutó en SRCDS ni se empaqueta en R9 |
| `82347b99…`, P5-R6–R8 | [T08 con SourceTVSupport](t08-coexist-srcds-acceptance.md), [auth/stats R6](p5-r6-auth-srcds-acceptance.md) y [rollback R7](p5-r7-rollback-srcds-acceptance.md); históricos |
| `bc76b00d…`, R3–R5 | HTTP/auth/stats aceptados en sus informes; SIGSEGV de salida observado en R5 y después corregido en candidatos nuevos |
| `cd42f40c…` | Cierre corregido, reinicialización con SourceTVSupport fallida; se conserva [incidencia](t08-reinit-srcds-acceptance.md), resuelta posteriormente |
| Baseline `9ca37aad…` | Rollback R7 con arranque/Steam/HTTP aceptados; conserva SIGSEGV al salir; bytes originales sin modificar |

R16 conserva [T11 de R14](interface-capabilities-srcds-acceptance.md): nueve familias
clasificadas, ocho interfaces disponibles y Matchmaking no proporcionada por el
contexto gameserver. Las rutas son los getters reales del candidato; la versión
es la permitida por el registro, sin afirmar negociación ni trazar cada rama de
caché/factory. GC local/GC999 rechazado una vez/defaults restaurados pasan con
144 checks aceptados; no se acredita entrega de mensajes del backend GC.

Se repiten 15 suites x86 y sanitizadores sin supresiones, tres consumidores,
catorce harnesses y ambos helpers. R16 incluye las fuentes del observador/helper de
capacidades, sin añadir diagnósticos al payload instalable. Extensión/include
coinciden byte a byte con R14 instalado; el rollback ZIP conserva el SHA ejercitado
en R9. El seguimiento T25 instaló el ZIP R15 verificado y restauró DEV.
R16 integra su evidencia/fuentes y genera nuevos paquetes solo localmente; no
operó el servidor en la integración original. El seguimiento posterior T32
sí instala el ZIP R16 verificado; ni R15 ni R16 se regeneran después. Los paquetes
anteriores y la [matriz R14 con T11](p5/history/p5-r14-with-T11-followup-validation-results.json)
permanecen intactos.

[T23](forward-consumers-srcds-acceptance.md), [T15/T29](identity-negatives-srcds-acceptance.md),
[T16](t16-extension-srcds-acceptance.md), [T20](t20-owner-srcds-acceptance.md) y
[T26/T35](p5-r9-rollback-srcds-acceptance.md) mantienen sus alcances. T29 sigue parcial;
el baseline conserva su fallo de salida. Dos casos abiertos:
**T25/T29** abiertos en ese checkpoint, antes del cierre posterior.

[T32 de R16](connection-srcds-acceptance.md) acepta un ciclo real acotado de
pérdida/recuperación Steam en el mismo proceso, con nueve getters y HTTP retenido/
nuevo preservados, 81 checks sin fallos. Filtros por cgroup/UID retirados y DEV
restaurado; no implica rendimiento sostenido, HTTP remoto, stats humanos ni GC.

[T33 de R16](soak-srcds-acceptance.md) acepta veinte minutos de carga HTTP local
mixta, con reposo/recuperación, métricas CPU/frame/latencia y límites de recursos
predefinidos. Mismo proceso, drenajes sin handles, quit limpio y DEV restaurado;
no certifica varias horas ni capacidad de producción. T25/T29 seguían parciales al cerrar T33; el seguimiento posterior los acepta en el alcance original.
