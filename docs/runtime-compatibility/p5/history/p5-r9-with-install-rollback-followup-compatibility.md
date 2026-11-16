# Registro de compatibilidad

P5-R9 es candidato local de revisión, sin publicación ni aceptación operativa global.
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

P5-R9 recompila el ejemplo, AoC y Family Share sin warnings y comprueba cuatro
tablas de imports, una precompilada. El inventario examina 2688 archivos y encuentra
34 con referencias. El corpus léxico y la carga de 138 plugins en el ensayo
optimizado no acreditan toda la semántica de consumidores; T25 permanece pendiente.

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
| Optimizado `839ff075…`, P5-R9 | [SRCDS directo](optimized-srcds-acceptance.md): dos reinits, 25 HTTP/18 bytes streaming, petición retenida, un humano con auth/identidad/stats y quit limpio; build R9 idéntico. T26/T35 con ZIP real aceptados en su seguimiento |
| Canónico `75a9b5d9…` | Build local pasa; no se ejecutó en SRCDS ni se empaqueta en R9 |
| `82347b99…`, P5-R6–R8 | [T08 con SourceTVSupport](t08-coexist-srcds-acceptance.md), [auth/stats R6](p5-r6-auth-srcds-acceptance.md) y [rollback R7](p5-r7-rollback-srcds-acceptance.md); históricos |
| `bc76b00d…`, R3–R5 | HTTP/auth/stats aceptados en sus informes; SIGSEGV de salida observado en R5 y después corregido en candidatos nuevos |
| `cd42f40c…` | Cierre corregido, reinicialización con SourceTVSupport fallida; se conserva [incidencia](t08-reinit-srcds-acceptance.md), resuelta posteriormente |
| Baseline `9ca37aad…` | Rollback R7 con arranque/Steam/HTTP aceptados; conserva SIGSEGV al salir; bytes originales sin modificar |

P5-R9 repite 14 suites x86 y sanitizadores sin supresiones, reproduce el optimizado
aceptado y regenera paquetes/captura por lectura. Se conserva la
[matriz anterior](p5/history/p5-r8-with-optimized-followups-validation-results.json)
y todos los ZIP previos. El [seguimiento R9](p5-r9-rollback-srcds-acceptance.md)
acepta T26/T35 con instalación real desde ZIP, reversión en el mismo árbol y
arranque/Steam/HTTP del baseline. Su SIGSEGV final queda separado. La
[matriz activa](p5/validation-results.json) mantiene los pendientes globales de
interfaces/GC/consumidores, identidad y lifecycle.
