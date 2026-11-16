# T25/T29 de P5-R16: aceptación Linux x86

Aceptados el 16 de noviembre de 2026 en `competitive_dev`, SRCDS x86 iniciado
mediante LinuxGSM, copia aislada en 27015 y sin contraseña añadida. Se instaló
el ZIP R16 verificado. La extensión sigue en `839ff075…` y el include público en
`a3e5a7a2…`; ninguna API pública cambia. [Resultados](last-two-resume-srcds/results.json)
y [matriz completa](p5/validation-results.json).

## T25: contratos de consumidores

Se conserva la aceptación de R15: tres SMX existentes y tres recompilaciones,
37 checks; dos controles nuevos con includes antiguo/actual, 22 checks humanos
H08/H09. Los controles no se presentan como binarios históricos.

La nueva cuenta autorizada con L4D2 prestado produjo el forward real de Family
Share Core y la distinción propietario/prestatario. El perfil del propietario
se consultó con este desconectado: persona no vacía, identidad/URL correctas y
una sola finalización, sin posibilidad de fallback al nombre de un cliente.
El grupo público se obtuvo del código AoC y el usuario confirmó su pertenencia.
La consulta SDK real respondió miembro y oficial. Se acreditan 12 checks sociales
sin fallos; el observador consume el callback real, sin afirmar que se ejecutó
el plugin AoC completo durante esta entrada. La clave Web API existente se leyó
privadamente en el contenedor, se inyectó solo en la convar protegida del laboratorio
y se eliminó el archivo temporal. No se exportaron clave, IDs, nombres ni tickets.
Los 1925 CFG compartidos conservan hashes y mtimes.

Connect corrige las dos llamadas HTTP antiguas con un lector interno común de
cinco argumentos. La validación previa conserva la cola ante un ACK de 2048
bytes y retira exactamente el mismo trabajo con un ACK válido. Los intentos con
un fixture POST inválido se conservan aparte en el informe de pausa.

Esta reanudación encontró además un fallo real de arranque en frío: Connect
confundía extensión registrada con contexto SDK listo y ejecutaba SetFailState.
Se conserva el fallo y su stack. La corrección separa presencia de biblioteca y
contexto vivo, espera al forward de conexión Steam y difiere el trabajo un frame.
Un único temporizador de reintento evita cadenas duplicadas; se cancela al cambiar
de mapa y descargar el plugin. No se suprime ninguna comprobación.

El SMX final `0b6bfaa5…` carga desde arranque en frío con seis proveedores reales
(Left4DHooks, Confogl, readyup, pause, Skills y Stats). Pasa GET de asignación HTTP200,
POST con ACK válido sobre trabajo sintético y cambio de mapa en el mismo proceso,
con otro HTTP200 después del nuevo mapa. Sin errores en la ejecución aceptada.
Tres harnesses ejecutan 104 checks deterministas en la VM real x86: lector 83,
contexto 16 y temporizador real 5. Los estados SDK del segundo son dobles controlados;
el arranque del producto completo aporta la evidencia real. Compilación sin warnings
y 66 pruebas Python correctas. `connect.inc` permanece idéntico a HEAD.

El criterio original T25 es que consumidores existentes precompilados y recompilados
cumplan los contratos acordados, incluidos H08/H09. Con la corrección de Connect
y estas ramas reales se satisface ese alcance. No equivale a certificar todo el
negocio de BanSystem/Connect, una partida completa o un Core remoto. Esas ampliaciones
no son requisitos de cierre de la matriz original; se conserva el estado parcial
anterior en historia. Los SMX antiguos que llamen al native HTTP con tres argumentos
siguen siendo rechazados y deben recompilarse con el contrato de cinco argumentos.

## T29: identidad antes y después de autenticación

La referencia serial se obtiene por la conversión pública de SourceMod, contrastada
con el proveedor realmente cargado. No se altera memoria ni autenticación. En
OnClientConnected se alcanza un cliente conectado, humano y no autorizado:
RequestStats devuelve false y SP_ERROR_NONE. Tras autenticarse, el mismo serial
acepta la consulta positiva. Seis checks pasan sin fallos. Se mantienen los
negativos bot/desconectado previamente aceptados con el mismo ELF.

El desensamblado del proveedor SourceMod `c475a8ac…` demuestra que AccountID y
SteamID64 validados proceden del mismo CSteamID y comparten la puerta de validación.
No puede aparecer normalmente AccountID validado no cero con SteamID64 cero o
contradictorio en esa llamada síncrona. Fallback y contradicción siguen probados
con los natives de producción en las suites deterministas x86. No se afirma haber
inducido esos estados en una cuenta real. El criterio T29 original es U/I y se
cierra combinando esas pruebas, la invariante binaria del proveedor soportado y
la entrada real pendiente→autorizada. Otros proveedores requieren su propia evidencia.
[Auditoría binaria](last-two-srcds/provider-identity-evidence.json).

## Integración, reversión y límites

Quince suites x86 repetidas pasan, incluidas comprobaciones con sanitizadores sin
supresiones; API pública verificada. No se repite una aceptación de instalación o
reversión ZIP: se conserva T26/T35 y el rollback R16 idéntico al ejercitado en R9.
Los cuatro ZIP R16 no se modifican. La corrección de Connect y esta evidencia se
entregan aparte como revisión, fuera del payload instalable de SteamWorks.

Quit controlado sin crash ni error en ambas fases aceptadas. DEV original vuelve
a arrancar con LinuxGSM, selector/módulo originales, Steam conectado y cuatro
hashes protegidos idénticos. Laboratorio retirado y fixture HTTP detenido.
[Restauración](last-two-resume-srcds/cleanup.json), contrastada por lectura posterior.
El contenedor estaba detenido al iniciar esta reanudación y queda encendido con
DEV restaurado, según el flujo de pruebas solicitado.

La matriz Linux x86 queda con 33 casos aceptados y tres no aplicables al perfil
(T13/T14/T34). No se acreditan Windows, x64, otro SDK/proveedor, GC remoto, varias
horas de carga ni descargas forzadas con callbacks activos. El baseline conserva
su SIGSEGV conocido al salir. Todo sigue en develop, sin commit, publicación ni
cambios de producción. Para distribuir Connect deberá usarse su recompilación
corregida; el ZIP fuente R16 histórico no contiene este parche externo posterior.
