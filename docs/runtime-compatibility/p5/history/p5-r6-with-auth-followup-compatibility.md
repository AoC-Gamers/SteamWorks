# Registro de compatibilidad

Estado de P5-R6: candidato local para revisión, sin autorización de publicación ni
aceptación operativa global. SDK objetivo 1.58a; no se cambió el SDK del juego.

| Plataforma/módulos | Ruta interna | Validación |
|---|---|---|
| Linux i386, L4D2 build 23990100, pareja identificada en P4 | Perfil `l4d2-23990100-linux-x86` | P5-R6: T08 con SourceTVSupport, dos reinits, HTTP/streaming y quit. Auth/identidad/stats humanos aceptados también en R6; matriz completa pendiente |
| Otra pareja Linux, aunque tenga el mismo nombre o versión visible | Rechazada | Sin perfil; requiere nueva evidencia |
| Linux x86_64 | Rechazada | Solo historia diagnóstica; no se genera ni entrega en P5-R3 |
| Windows | Rechazada | Excluido del candidato P5; flujo de build existente no certifica ABI |

Identidades, prefijos y evidencias completas:
[P4](p4-binary-profiles.md) y [perfil documental](p4/competitive-dev-linux-x86.json).
Los JSON no habilitan rutas del producto. No se distribuyen librerías del juego
ni del SDK con el candidato.

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

La revisión P5-R6 recompila el harness de ejemplo, AoC y Family Share sin warnings
y comprueba cuatro tablas de imports, incluida una precompilada. T25-I no queda
aprobado por ese resultado: la semántica completa de consumidores con el binario
actual sigue pendiente. El inventario P5-R6 examina 2683 archivos y encuentra 30
con referencias, incluidos harnesses. El corpus léxico no demuestra instalación
ni ejecución.

Se conservan dos observaciones de P0: faltan marcaciones opcionales para
`SetMapName` y `GetHTTPRequestWasTimedOut`, y existe el forward interno no declarado
`SteamWorks_BeginAuthSession`. [T36](t36-auth-forward.md) conserva su registro
`Param_Array` y el transporte compacto por `PushArray`, probado con CForward y VM
reales. El [adaptador 014](begin-auth-session-014.md) conserva un tipo separado de
la interfaz nativa 015. La [aceptación R3](auth014-srcds-acceptance.md) observó un
evento real y ambos forwards públicos sin inspeccionar tickets. Esa ejecución
corresponde a `ddfdf9bf…`. La [aceptación posterior del binario final](p5-r3-auth-srcds-acceptance.md)
repite directamente autenticación, identidad y stats enteros en `bc76b00d…`.
No se heredan aprobaciones entre ambos binarios.

La [aceptación float/missing-key de P5-R4](p5-r4-stats-srcds-acceptance.md) prueba
el mismo payload con dos rondas de 733 claves: 12 floats finitos e idénticos por
cliente/AccountID y clave inexistente rechazada conservando las cuatro salidas.
T03 incorpora ese alcance; los ZIP P5-R4 previos no se sobrescriben.

El candidato final `bc76b00d…` pasa la [aceptación HTTP streaming](http-stream-srcds-acceptance.md)
y una reconstrucción idéntica en P5-R5. Las 13 suites x86 y sanitizadores pasan;
las VMs instrumentadas siguen separadas del servidor. La aprobación global de
interfaces, consumidores y ciclo de vida restante se mantiene abierta conforme a
la [matriz P5](p5/validation-results.json). Windows y otras parejas quedan fuera.

La [reversión real de P5-R5](p5-r5-rollback-srcds-acceptance.md) recupera el
baseline desde el ZIP en el mismo árbol y pasa arranque/Steam/HTTP. Ambos procesos
registraron SIGSEGV al salir. El [seguimiento T08](t08-srcds-shutdown.md) identifica
la sesión cliente pendiente y corrige su cierre en un nuevo binario: tres ciclos
LinuxGSM x86 sin SIGSEGV ni nuevos cores. El baseline permanece intacto.
La [corrección de convivencia del init](t08-init-coexistence.md), con 14 suites
x86 y sanitizadores, pasa la [aceptación posterior con SourceTVSupport](t08-coexist-srcds-acceptance.md)
en el ELF `82347b99…`: dos reinicializaciones del mismo PID, HTTP/streaming,
respuesta antigua retenida y quit. El fallo del ELF anterior se conserva como
historia. T08 pasa en ese alcance; P5 continúa sin aceptación global.

P5-R6 empaqueta `82347b99…` y repite 14 suites. La evidencia humana de `bc76b00d…`
permanece histórica. El seguimiento humano posterior repite stats, identidad y
autenticación en el ELF nuevo y carga su payload del ZIP; la reversión R6 en sitio
todavía no se ejercita.

La [aceptación humana P5-R6](p5-r6-auth-srcds-acceptance.md) valida directamente
el payload del ZIP `82347b99…`: auth 014, ambos forwards públicos, identidad,
stats enteros, 12 floats por ambas rutas y claves ausentes con salidas intactas.
Cierra esa repetición y la instalación del ZIP; la reversión R6 en sitio sigue
pendiente.
