# Resolución de problemas

| Síntoma | Comprobación |
|---|---|
| No carga la extensión | Arquitectura x86, dependencias y errores de SourceMod; no copiar un SDK como runtime |
| Versión desconocida o detour rechazado | Identidad del módulo y perfil nativo; no forzar offsets ni activar JSON |
| HTTP 200 pero texto inválido | Retorno de lectura, tamaño exacto RawData y NUL; usar helper de cinco argumentos |
| Streaming pierde contexto | Extensión con regresión corregida y contexto de dos celdas del request |
| Stats solicitados pero sin valor | AppID del contexto, resultado asíncrono, identidad autorizada y key/tipo |
| Conteo SteamID parece uno mayor | Incluye NUL en el contrato legacy |
| GC disponible pero sin mensajes | Disponibilidad de interfaz no implica tráfico remoto |
| Callback tras un mapa/shutdown | Generación y propietario; no reutilizar requests invalidados |

El SIGSEGV al salir se relacionó con roles Steam sin teardown equilibrado; el
finalizador evita cerrar el rol del host en una descarga ordinaria y espera la
parada observada del GameServer. Las pruebas de reinicialización incluyen
coexistencia con SourceTVSupport. Una salida limpia de un perfil mínimo no
resuelve por inferencia todos los SIGSEGV del baseline ni valida todos los plugins.

La VM SourcePawn diagnóstica corrige la destrucción de un objeto derivado mediante
la ruta real de su tipo; los parches no desactivan ASan ni comprobaciones de
asignación/desasignación. Esa VM sirve para ensayos offline y no se instala en
SRCDS. Consulta las instrucciones de integración en las fuentes.
