# T25/T29 de P5-R16: seguimiento en curso

**Pausado por petición del usuario. Ambos casos siguen parciales.**
El laboratorio y los fixtures HTTP están detenidos y retirados. DEV original
fue restaurado mediante LinuxGSM y verificado conectado a Steam, con selector,
módulo y cuatro hashes originales. Los 1925 CFG de juego permanecen intactos.
[Checkpoint de pausa](last-two-srcds/pause.json).
[Resultados y fuentes](last-two-srcds/results.json) y [matriz](p5/validation-results.json).

Connect-Sourcepawn, en develop, corrige las dos llamadas antiguas y su declaración
local mediante un lector interno común de cinco argumentos. Rechaza tamaño
inválido, lectura fallida, truncamiento y conteos discordantes, sin entregar texto
parcial. Su API `connect.inc` y la API de SteamWorks permanecen intactas. Se
concilian los comentarios H08/H09. Las 66 pruebas Python y compilación pasan;
13 escenarios deterministas ejecutan 83 checks en la VM real x86, cero fallos.

El Connect real carga con Left4DHooks, Confogl, readyup, pause, Skills y Stats
reales. GET de asignación HTTP200 y los callbacks de ingestión se ejercitan contra
fixtures loopback, sin Core real ni partida. Un ACK de 2048 bytes conserva el
trabajo; el ACK válido retira después exactamente ese cuerpo. Los 1925 CFG
compartidos conservan SHA y mtime. El primer intento sin Confogl fue rechazado;
se aportaron dependencias, sin alterar su política. El primer fixture POST tuvo
un error Python de ruta: sus cuatro errores de transporte se conservan aparte y
no acreditan el ACK. Dos rechazos de ACK sobredimensionado posteriores son
negativos deliberados con logs normales; no se suprimieron comprobaciones.

El binario Connect reconstruido después de corregir comentarios solo difiere en
cuatro bytes de la hora de compilación; `.code` y el resto de secciones coinciden.
Se conservan hashes de ambos y el diff, sin atribuirles identidad binaria completa.
No se instaló Connect en producción ni se publicaron artefactos.

T29 tiene desensamblado del **proveedor SourceMod realmente mapeado**: ambos
getters validados leen el mismo CSteamID bajo la misma validación. Las alternativas
AccountID-only/inconsistente se cubren con escenarios deterministas del native,
sin fingir que sean estados inducidos de una cuenta real en este proveedor.
El nuevo observador usa la conversión pública de referencia serial para alcanzar
un cliente conectado/no bot/no autorizado, sin cambiar identidad, autorización o
memoria del motor. Aún no se ha observado esa entrada humana. El usuario confirmó que dispone
de la cuenta para realizar las pruebas al retomar.

Falta la cuenta con L4D2 prestado, el grupo autorizado y la ruta local de una clave
Steam Web API (la configuración protegida actual la tiene vacía). No enviar la
clave por chat. Perfil y grupo no se consultan hasta disponer de entradas reales.
El propietario solicitado para perfil debe estar desconectado: así una persona
no vacía demuestra respuesta Web API y no el fallback con nombre de cliente.
No se registran IDs, nombres, tickets ni la clave.

Quince suites x86 repetidas pasan. R16 y paquetes históricos intactos; sin commit,
publicación ni producción. La restauración preparada verifica todos los CFG,
hace quit controlado y devuelve LinuxGSM/selector/módulo/Steam a DEV antes de
retirar el laboratorio. Este informe es un punto de continuación, no aceptación
global ni una nueva certificación de todo el negocio de los consumidores.

## Retomar

Crear una nueva captura de laboratorio para conservar este checkpoint sin
sobrescribir sus evidencias. Verificar develop, hashes/API y estado original;
preparar la copia aislada en 27015 con LinuxGSM y sin contraseña añadida. Verificar
el SHA del proveedor antes de armar el observador. Connect debe permanecer
 descargado durante la entrada humana para evitar expulsiones por falta de
asignación. Cargar Family Core y el observador social con el gate de CFG.

Observar preautenticación real y control autorizado del mismo cliente; después
licencia prestada, perfil del propietario ausente y callback del grupo autorizado.
Faltan grupo de prueba y ruta local de clave Web API. No registrar IDs, nombres,
tickets ni clave. Registrar el alcance real antes de cerrar T25/T29 y restaurar
DEV de nuevo al finalizar. No hay reanudación o despliegue automático programado.
