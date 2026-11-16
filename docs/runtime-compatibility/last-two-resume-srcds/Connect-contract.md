# Contrato de lectura HTTP de SteamWorks

Las dos rutas de respuesta (asignación/lifecycle e ingestión competitiva) usan
`connect/http_body.inc`. El lector valida tamaño, capacidad incluido NUL, límite
de contrato y las salidas `written`/`truncated` antes de entregar texto completo.
Un fallo limpia el texto y el conteo; no se analiza una respuesta parcial ni se
retira una cola competitiva por un ACK incompleto. Una respuesta HTTP no exitosa
sigue siendo fallo aunque su cuerpo pueda leerse.

El include local SteamWorks declara los cinco argumentos del contrato actual.
No se modifica la API de la extensión ni `connect.inc`. Los consumidores antiguos
compilados con tres argumentos deben recompilarse para ese contrato; el proveedor
actual los rechaza expresamente. H08 mantiene AppID separado del AccountID y H09
cuenta el NUL; los comentarios locales reflejan esas reglas.

Validación local: `python3 -m unittest discover -s tests`, `make build-smx` con
salida `.build` nueva y compilación de `tests/sourcepawn/http_body_contract_test.sp`
con `-iaddons/sourcemod/scripting` y los includes de SourceMod. El harness queda
fuera del artifact y ejecuta 13 escenarios deterministas, 83 checks, sin I/O HTTP.
Los proveedores mock se usan únicamente dentro de ese harness; incluye el lector
real de producto y comprueba tamaños, fallos, truncamiento y conteos discordantes.

Seguimiento runtime en el laboratorio SteamWorks P5-R16: Connect recompilado carga
con Confogl/readyup/pause/Stats/Skills/Left4DHooks reales. GET de asignación local
HTTP200 y POST de ingestión con trabajo sintético ejercitan los callbacks reales.
ACK de 2048 bytes se rechaza conservando el archivo; ACK válido retira el mismo
contenido. No se prueba un receptor Core real, una partida ni datos de usuarios.
El primer fixture POST tuvo un error de ruta Python y está separado de los
ensayos válidos. No se ocultaron sus errores de transporte ni los rechazos
negativos deliberados. Esto no certifica todo el negocio/dependencias de Connect.

El arranque en frío detectó un segundo fallo real: `SteamWorks_IsLoaded()`
representa el contexto SDK y puede ser falso aunque la biblioteca de la extensión
ya esté registrada. `Runtime_HasSteamWorks()` comprueba presencia; `Runtime_IsSteamReady()`
comprueba el contexto vivo antes de I/O. El forward `SteamWorks_SteamServersConnected`
difiere un frame la reanudación. Los callbacks de biblioteca solo mantienen flags.
El temporizador de asignación tiene un solo propietario, reemplaza el reintento
anterior y se cancela en OnMapEnd/OnPluginEnd para evitar cadenas duplicadas.

Tres fixtures SourcePawn incluyen los helpers reales de producto: lector HTTP
(83 checks), inventario/contexto (16, SDK simulado explícitamente) y temporizador
(5, timers SourceMod reales). Los 104 checks pasan en SRCDS x86. El SMX final
`0b6bfaa581f2876c5034ea5f31a01b2882107d4b6acd9fbec2662183d60b088a`
pasa arranque en frío con seis proveedores reales, GET200, ACK válido que retira
el trabajo sintético y cambio de mapa con GET200 posterior. La ejecución aceptada
no tiene errores; el SetFailState previo y el intento con temporizadores duplicados
se conservan por separado en la evidencia SteamWorks. Las 66 pruebas Python pasan.

El seguimiento humano T25/T29 de SteamWorks se completó en el alcance de sus
contratos: préstamo, perfil Web API, grupo y preautenticación real. No acredita
una partida ni un receptor Core remoto. DEV fue restaurado; cambios sin commit,
publicación o producción. No distribuir claves, headers ni CFG privados.
