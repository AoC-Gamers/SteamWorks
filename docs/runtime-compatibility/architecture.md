# Arquitectura y contratos integrados

La API SourcePawn permanece en `scripting/include/steamworks.inc`: 47 natives,
11 forwards declarados, cinco enums y cuatro typesets. El inventario P0 y
`scripts/test-integration.py` comparan declaraciones, registros, valores y
opcionalidad, ignorando únicamente posiciones de línea y metadatos de captura.

La frontera `swpawn.h` comprueba las conversiones de SourcePawn; `swtypes.h`
separa AccountID, AppID y tamaños. `swidentity.cpp` conserva la conversión textual heredada; la identidad
validada de stats se resuelve en `ssnatives.cpp`. Estas comprobaciones no certifican la capacidad completa
de un buffer proporcionado por el plugin: el consumidor debe declarar su tamaño real.

`swinterfaces.h` vincula cada familia a una declaración SDK exacta. El resolver
rechaza un override desconocido antes de usar la factoría. La lista de versiones
compilables no demuestra que todas estén disponibles o funcionen en L4D2.

El perfil interno valida la pareja engine/API, el punto de llamada de init del
motor y la entrada de shutdown. El [observador de init](t08-init-coexistence.md)
parchea el `CALL` del motor y conserva el export compartido del SDK para
SourceTVSupport. La biblioteca adquirida pertenece al observador hasta restaurar
ese `CALL` y destruir el trampoline de shutdown. El inicio recorre la cadena
original; solo su éxito inicia consumidores.
La parada invalida recursos antes de llamar al shutdown original. El selector
usa build-id, RVA, región ejecutable y prefijos; los SHA documentales identifican
archivos y no se recalculan como comprobación de integridad en el proceso.

GameServer y GC poseen sus identificadores de hooks y receptor. Los callbacks
globales se desregistran antes de liberar forwards. Cada request HTTP conserva
su interfaz creadora, identidad de plugin y generación. El cierre dentro de un
callback difiere la destrucción hasta salir del despacho; el cierre del runtime
invalida requests anteriores. No se adquiere una interfaz nueva para liberar un
recurso creado por la anterior. Véase [P3](p3-resource-lifecycle.md).

Cada petición también conserva el contexto completo tras un setter aceptado:
headers/data usan ese valor ante la pérdida de la celda alta en el proveedor x86.
Completion conserva el call result. Véase la [corrección de streaming](http-stream-context.md).

| Capacidad | Contrato vigente | Evidencia y límite |
|---|---|---|
| Stats | Publicar salidas solo en éxito; AppID del contexto; AccountID individual | U; candidato final acepta enteros, 12 claves float y clave inexistente sin modificar salidas, por ambas rutas |
| Identidad | Humanos autorizados; SteamID textual cuenta NUL para capacidad positiva | U con AMTL/SDK; humano real, buffers, bot/desconectado y preautenticación; alternativas defensivas deterministas e invariante del proveedor en T29 |
| IPv4 | Discriminar familia; preservar 32 bits | U con ambos signos de char; consulta de estado P4 |
| HTTP | Cinco argumentos del helper textual; bytes separados de terminador; contexto completo por petición y propiedad por generación | U y ASan; candidato final acepta HTTP/TLS/timeout/streaming y mapa en SRCDS x86 |
| GC | Tamaños válidos y resultados sin memoria no escrita | U + T11: interfaz y operaciones locales aceptadas; protocolo/backend remoto no aceptado |
| Forwards | Generación y desregistro; autenticación como bytes compactos vía `PushArray`, sin copyback | U + [T36 con CForward/VM reales](t36-auth-forward.md); evento y validación reales; T23 acepta cero/uno/varios consumidores y resultados con argumentos sintéticos, sin inferir todos los eventos SDK |
| Lifecycle | Pareja específica Linux x86, original preservado y parada ordenada | U + ejecución aislada P4; no certifica otras parejas |

Desconexión Steam y destrucción del runtime son eventos distintos. El estado
`IsLoaded` por sí solo no acepta ninguna de estas capacidades.

Estado vigente y límites: [P5-R17](p5-integration.md) y [matriz de aceptación](p5/validation-results.json).
