# Contexto de callbacks HTTP streaming en Linux x86

La corrección conserva las dos celdas del contexto en headers/data. Está validada
localmente con pruebas deterministas y sanitizadores. La
[aceptación posterior en SRCDS x86](http-stream-srcds-acceptance.md) pasa para este
binario en un laboratorio LinuxGSM aislado. Este informe y su JSON conservan la
validación local previa, cuyo campo `srcds_acceptance: pending` describe ese
checkpoint. El fallo registrado en P5-R2 conserva su valor histórico.

## Causa y evidencia

Se reprodujo la pérdida fuera de SourceMod: una sonda ELF32 carga una copia del
proveedor real, adquiere `SteamClient020` y `STEAMHTTP_INTERFACE_VERSION` mediante
un usuario/pipe local, y consulta exclusivamente una fixture HTTP en loopback.
No carga la extensión, inicia SRCDS, hace logon ni autentica una cuenta. Se ejecutó
en un contenedor con `--network none`, HOME y directorio de trabajo temporales.
Las copias se obtuvieron mediante lectura de `competitive_dev`.

El setter acepta el contexto sintético `0x0000000712345678`. Los eventos recibidos
directamente por `Steam_BGetCallback` contienen:

| Evento | Tamaño x86 | Contexto recibido | Otros campos |
|---|---:|---|---|
| Headers 2102 | 12 | `0x0000000012345678` | Handle correcto |
| Data 2103 | 20 | `0x0000000012345678` | Offset 0, longitud 3 |
| Data 2103 | 20 | `0x0000000012345678` | Offset 3, longitud 3 |
| Completion 2101, obtenido por call result | 24 | `0x0000000712345678` | Éxito, HTTP 200 |

La estructura x86 tiene el contexto en offset 4, el offset del fragmento en 12 y
su longitud en 16. Los tamaños/campos coinciden con SDK 1.58a y el payload anterior
de SRCDS era correcto. **El proveedor pierde los 32 bits altos en los eventos de
streaming**, antes del despacho de la extensión; no es una inversión de celdas ni
un cambio de packing. El mismo resultado se observó con ambas copias:

- `serverfiles/bin/steamclient.so`: SHA-256 `80f6c6f041fb187daff229aeb26b9112ea18dc76edab211d2cfea74cc022ecec`.
- `.steam/sdk32/steamclient.so`: SHA-256 `c4e495a2bbc4f23e63f2b437cb64107cd63d7c7f79d04791cba0ef71022491bc`.

La extensión pasaba directamente `m_ulContextValue` de esos eventos al forward.
Las pruebas anteriores de ciclo de vida comprobaban la aridad, no sus dos celdas.
No se atribuye aquí una instrucción interna concreta del proveedor ni se presume
que todas sus versiones tengan el mismo defecto.

## Corrección

Cada `SteamWorksHTTPRequest` conserva su contexto completo, inicialmente cero.
`SetContextValue` llama a la interfaz que creó la petición y actualiza el valor
local solamente si Steam devuelve éxito. Un setter rechazado conserva el último
valor aceptado. Las peticiones detenidas no acceden al proveedor.

Headers y data siguen filtrando handle y generación, y envían el contexto local
completo, incluso cuando vale cero. No se usa una heurística sobre los bits del
evento ni un estado global compartido entre peticiones. Completion conserva su
lectura del call result. No cambian natives, firmas públicas, orden de argumentos,
offsets, longitudes, flags de error ni reglas de cierre/desregistro.

## Validación local

`bash scripts/test-http-stream-context.sh` compila obligatoriamente para x86
(`-m32` por defecto y `static_assert(sizeof(void*) == 4)`). Ejecuta producción:
natives HTTP, manager/request y clases de callbacks del SDK. El servicio de
registro Steam y el host SourceMod se simulan. La fixture binaria se escribe en
little endian independientemente de los constructores del SDK y reproduce el
contexto truncado observado.

- 92 comprobaciones: ambas celdas y bits de signo, cero explícito/default,
  setter inicial y posterior rechazados, fragmentos 0/3, handles ajenos,
  peticiones simultáneas, completion, parada y reutilización de handle.
- Control negativo: restaurar la lectura directa del evento falla en la celda
  alta de headers, sin usar un fallo de sanitizador como resultado esperado.
- La misma prueba y control negativo pasan con Clang 14 x86 + ASan/LSan/UBSan,
  sin supresiones. Las seis suites de ciclo de vida también pasan con ese entorno.
- Integración x86: 11 suites, incluida la nueva regresión y el enlace SourceHook
  de autenticación. La comparación contra P0 confirma la API pública.
- Compilación L4D2 estricta: ELF32/i386, dependencia portable `libtier0_srv.so`.

[Resultados y hashes](http-stream-context/results.json),
[integración](http-stream-context/integration.json),
[proveedor de serverfiles](http-stream-context/provider.log),
[proveedor sdk32](http-stream-context/sdk32-provider.log),
[sanitizadores](http-stream-context/asan.log) y
[ciclo de vida](http-stream-context/asan-lifecycle.log) permiten revisar el alcance.
La sonda y su runner están en `tests/diagnostics/`; las instrucciones están en su
README. Binarios, logs completos y build permanecen en `.build/http-stream-context/`.

## Seguimiento de aceptación

La [ejecución posterior autorizada](http-stream-srcds-acceptance.md) usó la
fixture/harness original en SRCDS x86 y verificó contexto `(7, 0x12345678)` en
headers y ambos fragmentos, completion correcto, seis bytes por respuesta y
ninguna falla HTTP. No se debilitaron sus asserts. Los ZIP e informes históricos
de P5-R2/R3 no se regeneraron; la aceptación acotada no aprueba P5 globalmente.
