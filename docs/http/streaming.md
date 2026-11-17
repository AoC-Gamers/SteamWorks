# HTTP streaming

`SendHTTPRequestAndStreamResponse` separa cabeceras, fragmentos y finalización.
El offset y tamaño del callback de datos describen bytes disponibles; consulta
`GetHTTPStreamingResponseBodyData` para ese rango. Un chunk no es el body entero
ni una cadena NUL y puede dividir texto multibyte.

Cabeceras y datos reciben el **contexto de dos celdas almacenado por el request**.
La ABI de los callbacks streaming que entrega el runtime real no permite reutilizar
sin adaptación la posición del contexto del SDK actual. La corrección despacha
`m_contextValue` de la solicitud, que ya conserva ambas celdas, en vez de leer
`pRequest->m_ulContextValue` en esos dos callbacks.

El fixture independiente modela el wire del runtime y compara dos requests con
contextos distintos, tamaños, offsets, payloads y cierre/reentrada. Su control
negativo restaura la lectura del campo equivocado y debe fallar por pérdida del
contexto. No reemplazar el fixture por el struct del mismo SDK que se prueba:
se perdería la independencia que detectó el error.

`test-http-stream-context.sh` ejecuta esa regresión en x86. La aceptación SRCDS
histórica tiene su propio hash y alcance en la cobertura. Reinicialización,
descarga del propietario y callbacks tardíos mantienen las reglas de
[propiedad](callbacks.md). El helper textual de respuesta completada no es un
lector de streaming.
