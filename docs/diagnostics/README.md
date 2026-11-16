# Diagnóstico

`steamworks_debug` es una máscara, desactivada por defecto:

| Bit | Categoría |
|---|---|
| 1 | Lifecycle |
| 2 | Interfaces |
| 4 | Hooks |
| 8 | Autenticación |

`steamworks_debug 12` combina hooks y autenticación; 15 selecciona todas.
Actívala solo durante la observación y vuelve a cero al terminar. El backend
escribe en el directorio de logs SourceMod con rotación limitada; revisa eventos,
capacidad disponible y generación, sin registrar tickets, claves o IDs personales.
Apagar el debug evita abrir/formatear un log; no suprime validaciones funcionales.

Distingue entrada al hook, despacho del forward, resultado agregado y callback
posterior: ninguno por sí solo acredita todos los pasos de autenticación.
Los eventos de resolución describen la interfaz obtenida, no una aceptación
universal de sus métodos. Errores de perfil o detours no se solucionan habilitando
un JSON documental.

`test-debug-log.sh` valida máscara, rotación, reactivación y fallo de I/O con
dobles. Los probes de fuentes son herramientas diagnósticas, no extensiones para
instalar. Consulta [problemas conocidos](troubleshooting.md).
