# T25 de P5-R15: consumidores e identidad en SRCDS x86

**Resultado parcial.** Se instalaron los archivos del ZIP candidato R15 verificado
en una copia aislada de `competitive_dev:27015`, iniciada por LinuxGSM con la
configuración habitual y sin contraseña añadida. Se conservan API pública y
binario optimizado `839ff075…`; el include mantiene `a3e5a7a2…`.
[Resultados y hashes](consumer-compat-srcds/results.json),
[procedencia de consumidores](consumer-compat-srcds/consumers.json) y
[matriz vigente](p5/validation-results.json).

## Consumidores reales

| Consumidor | Precompilado existente | Recompilado con include actual | Ruta ejercitada en ambos |
|---|---|---|---|
| Ejemplo SteamWorks | SMX conservado de P5-R2 | Fuente actual sin cambios | HTTP loopback: una completion, éxito, HTTP 200, cuerpo de 16 bytes y cierre del handle |
| AoC | SMX conservado de P5-R2 | Fuente actual sin cambios | Forward público de conexión invocado sobre el plugin real; IP consumida coincide privadamente con el native, metadatos restaurados |
| Family Share Core | SMX existente de su directorio dist | Fuente actual sin cambios | Biblioteca/natives, rechazo de cliente 0 y fallback local asíncrono para una cuenta sintética: un callback, perfil/persona vacíos según contrato |

Las seis variantes cargan y descargan; **37 comprobaciones pasan sin fallos**.
Sus imports no tienen natives SteamWorks registrados ausentes. El observador bloquea
cuatro ejecuciones de los CFG de AoC/Family Share mediante sus rutas exactas.
Los archivos originales conservan SHA y mtime durante las variantes; se usan
valores por defecto y dependencias opcionales ausentes. Esto no acredita sus
configuraciones de producción, confogl/readyup/BanSystem, licencia prestada real,
perfil Web API positivo ni grupos. El callback AoC se despacha desde el harness;
no se atribuye a una nueva conexión Steam espontánea. Se restauran inmediatamente
`hostip` y `net_public_adr`, sin exportar IP.

## H08/H09 con usuario autorizado

Dos controles **nuevos**, compilados respectivamente con el include archivado
`e51bcd39…` y el actual, pasan **11 checks cada uno, 22 en total**. Son fixtures
recién compilados; se distinguen de los tres SMX preexistentes de la tabla.

- Identidad igual a la validada por SourceMod, comparada privadamente; longitud
  incluye NUL, truncamiento de capacidad 3, capacidad 1 con NUL y canario intacto,
  capacidad 0 sin escritura y retorno 0.
- `RequestStats` por cliente y AccountID acepta AppID 550 y rechaza 0/1.
  La aceptación de la solicitud asíncrona se distingue de la disponibilidad:
  tras revalidar userid, ambas lecturas de `Stat.GamesPlayed.Total` tienen éxito
  y coinciden. El observador limita la espera a 20 segundos; ambas variantes
  completaron en la primera observación a los tres segundos.
- Las claves inexistentes conservan sentinelas distintos en ambas rutas.

Se consultaron stats en modo de solo lectura. No se exportaron IDs, tickets ni
valores personales. Los consumidores reales ya estaban descargados al entrar
el usuario. Total: **59 comprobaciones positivas, cero fallos**.

## Incompatibilidad externa reproducida

Connect conserva dos llamadas de tres argumentos a
`SteamWorks_GetHTTPResponseBodyString`, en `connect/http.inc:224` y
`connect/ingest.inc:569`; el contrato público exige cinco.
[Fuentes y hashes externos](consumer-compat-srcds/external-connect-findings.json).
El control de compilación con el include actual produce error 034 por el cuarto
argumento ausente. Un fixture con la declaración antigua y un handle HTTP válido
sin enviar recibe `SP_ERROR_NATIVE` 23, conserva su buffer y cierra el handle.
Se repite dos veces y ambas excepciones se registran normalmente: no se suprime
ninguna comprobación ni log. No se cargó ni modificó Connect completo.

El primer recolector clasificó incorrectamente la atribución
`disabled/legacy_http.smx`, pues esperaba solo el basename. Se conserva ese
[primer resultado](consumer-compat-srcds/first-collector-legacy-arity.json).
La corrección admite únicamente las dos rutas exactas del fixture y verifica
native y mensaje esperados. El [segundo ensayo](consumer-compat-srcds/legacy-arity-confirmed.json)
y la [auditoría posterior al quit](consumer-compat-srcds/post-quit-log-audit.json)
confirman dos excepciones deliberadas y cero errores inesperados.

T25 permanece parcial por estas incompatibilidades externas y por los flujos
completos de negocio/dependencias aún no cubiertos, incluidos BanSystem/Connect
y licencia prestada/Web API/grupos de Family Share. No se extrapola una carga
correcta a todos los métodos o escenarios del consumidor.

## Cierre y entrega

Las variantes y la observación humana conservaron PID 140518/start ticks 3680667.
HTTP final y quit controlado pasan sin marcadores de crash. Se retiró el
laboratorio y el fixture loopback, se restauró el selector LinuxGSM original y
se inició DEV original (PID 143996 al cierre), conectado a Steam y con su extensión
original mapeada. Los cuatro hashes protegidos se conservan:
[restauración](consumer-compat-srcds/cleanup.json).

Se repiten **15 suites x86** con éxito y se compilan sin warnings los consumidores
y fixtures. Los sanitizadores de VM se ejecutan offline, sin supresiones; no se
instala ASan en SRCDS. [Validación](consumer-compat-srcds/integration.json).

Los cuatro ZIP R15 y los paquetes históricos permanecen intactos. Este seguimiento
se entrega separado en
`.build/consumer-compat-srcds/review/steamworks-p5-r15-consumer-compat-srcds-validation.zip`,
con evidencia seleccionada, fuentes propias y manifiesto/checksums; sin payload
addons, binarios/includes ajenos, CFG privados ni logs crudos de consola.
Las recetas y dependencias externas están en [tests/runtime](../../tests/runtime/README.md).
No constituye una regeneración global de P5 ni una nueva aceptación de reversión.
Siguen abiertos **T25, T29, T32 y T33**. Rama develop, sin commit, publicación ni
operación en producción.
