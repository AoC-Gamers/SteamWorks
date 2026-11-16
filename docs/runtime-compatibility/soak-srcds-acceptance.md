# T33 de P5-R16: operación sostenida y recursos

**Aceptado en el alcance del plan registrado antes del ensayo.** El ZIP R16
verificado se instaló con SRCDS detenido en la copia aislada de
`competitive_dev:27015`, iniciada por LinuxGSM con la configuración habitual y
sin contraseña añadida. El ELF conserva `839ff075…` y el include `a3e5a7a2…`.
API e implementación intactas. [Plan y umbrales](soak-srcds/plan.json),
[resultados](soak-srcds/results.json) y [matriz](p5/validation-results.json).

## Duración y carga

Tras calentamiento: **3 minutos de reposo, 20 minutos de carga y 3 minutos de
recuperación**, en el mismo proceso. Cada minuto de carga alterna 50 segundos de
solicitudes a intervalos de 0,5 s y 10 segundos de drenaje. Ocho slots máximos;
pico observado 3. Se alternan texto 200, streaming binario con
NUL/0xff, HTTP503 esperado y respuesta demorada tres segundos, todo por loopback.

Se completaron y cerraron **1980 solicitudes sin fallos**. Los callbacks
verifican contexto/secuencia, status, cuerpo, chunks sin duplicados y headers del
stream. 2970 bytes en 990 callbacks de streaming.
Los contadores sent/completed/closed coinciden y los handles HTTP visibles vuelven
a cero en cada muestra de drenaje, reposo y recuperación. También son cero los
handles propios totales del harness, incluidos timers, en esas muestras.

## Mediciones y criterios

28 muestras agregadas, con PID/start ticks, módulos mapeados,
hashes/mtime de archivos protegidos y conexión Steam conservados. Se muestrean
/proc, dump de handles y estado del harness; el dump crudo se elimina después de
contar por propietario/tipo. No se exportan tickets, IDs ni datos personales.
[CSV](soak-srcds/resources.csv) y [muestras](soak-srcds/samples).

| Recurso | Referencia de reposo | Final | Criterio predefinido |
|---|---:|---:|---|
| RSS | 330.11 MiB (mediana) | 330.66 MiB | Delta ≤ 16 MiB; observado 568 KiB |
| Descriptores | 53 | 53 | Delta ≤ 2 |
| Hilos | 12 | 12 | Delta ≤ 2 |
| Pendiente/handles HTTP | 0 | 0 | Cero en todos los drenajes |
| Pendiente RSS últimos 10 minutos de carga | — | 0.000 KiB/min | ≤ 128 KiB/min |
| Pendiente FDs últimos 10 minutos de carga | — | 0.000/min | ≤ 0/min |

CPU del proceso: reposo 7.46 %, carga
7.62 %, recuperación
7.36 %, expresados como porcentaje de un
núcleo. Media de frame: reposo 33.333 ms y carga
33.333 ms; máximo durante carga 36.254 ms.
Son medidas del proceso y sus ventanas, no una comparación con el binario baseline
ni un SLO de capacidad de producción.

| Ruta | Completions | Latencia media ms | Cota p95 ms | Máximo ms |
|---|---:|---:|---:|---:|
| Texto 200 | 495 | 83.4 | ≤ 250 | 136.4 |
| Streaming binario 200 | 495 | 216.2 | ≤ 1000 | 268.5 |
| HTTP 503 esperado | 495 | 83.1 | ≤ 250 | 135.1 |
| Delay de 3 s | 495 | 3079.4 | ≤ 5000 | 3135.6 |

El p95 es una **cota del histograma**, no un percentil calculado con muestras
individuales. Los límites previos fueron 1000 ms para rutas no demoradas y 5000 ms
para delay; pasan. El harness emite únicamente agregados por snapshot y un aviso
por burst, sin logs por petición/frame. Los logs estándar de la extensión no
crecen durante las ventanas y no aparecen logs de errores.

## Cierre y validación

No hubo cambios de mapa, recargas ni reinicios durante las ventanas. Steam
figura conectado en cada muestra; no se generó ningún corte de red.
El cierre de cada petición se realiza en su completion. Tras drenaje y snapshot
final, el harness desarma expresamente la vigilancia de map_end para el quit
controlado autorizado. Timer y slots propios se liberan al descargar. El quit no
produce crash ni errores y los contadores finales permanecen consistentes.
[Cierre](soak-srcds/shutdown.json).

Se restauraron selector LinuxGSM, cuatro hashes protegidos, módulo y DEV conectado;
laboratorio y fixture retirados. No se aplicó ningún filtro de red.
[Restauración](soak-srcds/cleanup.json). Las 15 suites x86 repetidas pasan con
sanitizadores offline sin supresiones; observer sin warnings. No se instala ASan
ni la VM instrumentada en SRCDS. [Suites](soak-srcds/integration.json).

## Límites y entrega

Se acepta este ensayo de veinte minutos de carga HTTP local, con comparación
reposo/carga/recuperación. No acredita operación de varias horas, capacidad con
jugadores humanos, HTTP remoto, GC o stats positivos del backend. RSS/PSS y
FDs/hilos de todo SRCDS no demuestran ausencia absoluta de fugas del allocator o SDK.
Los umbrales no se ajustaron después de observar los resultados.

ZIP R16 y paquetes históricos intactos. Evidencia, CSV, fuentes propias y recetas
se entregan separados en
`.build/soak-srcds/review/steamworks-p5-r16-soak-srcds-validation.zip`, con
manifiesto/checksums; SMX diagnóstico fuera de addons, sin CFG privados, SDK ni
logs crudos. [Receta](../../tests/runtime/README.md).
Siguen abiertos **T25 y T29 parciales**: P5 aún no tiene aceptación global.
Develop, sin commit, publicación ni producción.
