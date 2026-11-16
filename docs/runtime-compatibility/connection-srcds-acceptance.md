# T32 de P5-R16: pérdida y recuperación de Steam

**Aceptado en el alcance de un ciclo acotado real.** El ZIP R16 verificado se
instaló con SRCDS detenido en la copia aislada de `competitive_dev:27015`, iniciada
por LinuxGSM con configuración habitual y sin contraseña añadida. El candidato
conserva ELF `839ff075…` e include `a3e5a7a2…`; no se modificó API ni implementación.
[Resultados/hashes](connection-srcds/results.json),
[instalación](connection-srcds/setup.json) y [matriz](p5/validation-results.json).

## Ensayo observado

| Fase | Resultado |
|---|---|
| Inicio real | Un forward conectado del SDK; ocho interfaces presentes y Matchmaking ausente, nueve getters auditados; respuesta HTTP retenida |
| Corte de egreso | 51,52 segundos de DROP; 22 paquetes IPv4/1575 bytes descartados; regla IPv6 presente, sin descartes |
| Pérdida Steam | Forward real desconectado, reason 3; `IsConnected=false`, `IsLoaded=true`. Observado a los 27,63 s del inicio del sondeo |
| Durante el corte | Los nueve resultados de getters son iguales al baseline, respuesta retenida legible, stats inválidos conservan salida; nueva petición HTTP loopback completa con cuerpo/contexto correctos |
| Restauración de red | Retirada explícita de saltos y cadenas propios; segundo forward conectado real y `IsConnected=true`, observados a los 3,12 s del sondeo de recuperación |
| Tras recuperar | Mismos getters y respuesta retenida; nueva completion HTTP/contexto/cuerpo; 81 checks acumulados, cero fallos y tres completions en total |

No se despacharon forwards sintéticos ni se llamó LogOff/LogOn, init, shutdown,
cambio de mapa o reload durante la pérdida/recuperación. Se conservaron PID
147982, start ticks y las identidades de módulos mapeados durante todo el ciclo.
Los getters proceden del candidato real, preparados mediante SDKCall_Raw tras
validar SHA/inode, rangos ejecutables y slot escribible del resolver con el perfil
auditado de T11. Sus valores se comparan en memoria, sin publicar punteros.
La respuesta retenida mantiene su handle hasta descargar el harness al terminar.
Los tiempos de detección son observaciones de sondeo, no latencias exactas del SDK.

## Aislamiento y retirada

El contenedor utiliza red host. El guard temporal solo añade saltos OUTPUT con
**cgroup exacto de competitive_dev y UID 1000 de LinuxGSM**, en IPv4/IPv6.
El laboratorio tiene sus plugins ajenos en cuarentena y un único SRCDS.
Loopback, egreso de juego desde 27015 y DNS quedan permitidos. Se descarta el resto
del egreso coincidente; el filtro no pretende identificar destinos Steam y no se
aplica a otros cgroups del host. No se vacía OUTPUT ni se tocan cadenas ajenas.
[Ámbito](connection-srcds/network-scope.json),
[preflight sin tráfico bloqueado](connection-srcds/network-preflight.json) y
[contadores del guard](connection-srcds/guard-events.json).

El guard tiene retirada ante excepción, señal y límite temporal de 210 s desde
el bloqueo. Este ensayo acredita la retirada explícita; no ejercita la expiración
del watchdog. Una comprobación independiente confirma ausencia de ambas cadenas
y sus saltos: [verificación de red](connection-srcds/network-restored.json).

Después de la reconexión se hizo quit controlado: sin crash ni logs de errores,
con los 81 checks pasando al descargar. Se restauraron selector LinuxGSM, cuatro
hashes protegidos, módulo y DEV conectado; laboratorio y fixture retirados.
Los fingerprints de archivos protegidos, incluidos mtime, permanecen iguales
entre baseline, corte y recuperación. El hash original del selector se restaura
al cierre; no se afirma restauración de su mtime original.
[Cierre](connection-srcds/shutdown.json), [restauración](connection-srcds/cleanup.json).

## Validación y límites

Quince suites x86 repetidas pasan, incluida VM con sanitizadores offline sin
supresiones. El observer compila sin warnings. No se instala VM/ASan en SRCDS.
[Suites](connection-srcds/integration.json) y
[compilación](connection-srcds/observer-compile.log).

Se acepta un ciclo breve de pérdida/recuperación y conservación del runtime,
interfaces y recursos HTTP. HTTP probado es loopback, no un servicio remoto.
No se infieren stats humanos positivos después del corte, entrega GC, todos los
servicios Steam, pérdida indefinida ni rendimiento/carga sostenida de T33.

Los ZIP R16 y los históricos permanecen intactos. Fuentes propias, harness SMX
fuera de addons, recetas y evidencia seleccionada se entregan separadas en
`.build/connection-srcds/review/steamworks-p5-r16-connection-srcds-validation.zip`,
con manifiesto y checksums. No se exportan CFG privados, logs crudos de consola,
tickets, IDs o valores personales. [Receta](../../tests/runtime/README.md).
Siguen abiertos **T25 y T29 parciales, T33 pendiente**. Develop, sin commit,
publicación ni producción.
