# Callbacks y propiedad HTTP

Un request pertenece a la identidad del plugin que lo creó y a una generación
Steam. No se puede usar un handle ajeno ni uno invalidado por shutdown. Después
de reinicializar hay que crear otro request; no reasignar un handle antiguo.

El contexto público tiene **dos celdas**. Se preserva como valor aceptado por el
request y se entrega con ambas celdas a sus callbacks. No reducirlo a un entero
local ni reconstruirlo a partir de un campo con ABI distinta.

Los call results y callbacks se cancelan al descargar al propietario o detener
el runtime. Se comprueba vigencia antes de despachar y después de reentrada.
Cerrar el handle desde un callback es admisible: la destrucción se difiere hasta
salir del despacho para no destruir el objeto que está ejecutando el método.
Callbacks tardíos o duplicados no deben revivirlo.

Los lectores deben comprobar retorno además de código HTTP. El proveedor puede
completar con error de transporte, timeout o respuesta sin datos. Un resultado
200 no garantiza buffer válido ni texto terminado. No se publica una salida
parcial del helper textual si falla la lectura completa.

Las suites `test-resource-lifecycle.sh`, `test-typed-boundary.sh` y
`test-http-stream-context.sh` usan producción con dobles. Los harnesses de
propietario pendiente y cierre desde callback requieren SRCDS explícito.
