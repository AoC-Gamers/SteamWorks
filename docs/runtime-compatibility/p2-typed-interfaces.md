# P2: frontera tipada y registro mínimo de interfaces

Actualización: el [adaptador tipado 014](begin-auth-session-014.md) separa el
contrato de autenticación del contrato 015 de natives. Este documento conserva
su checkpoint anterior; los resultados nuevos están en el informe del adaptador.

Fecha: 2026-11-14. Rama `develop`, base `46e3ee05fc29ad090a1d6f7b1cb5e3dca9855350`.
Se conservan todos los cambios locales de documentación, P0 y P1-A/B/C/D/E.
Sin commit, publicación ni despliegue. Validación unitaria y compilación de
objetos; aceptación del binario en SRCDS pendiente.

## Frontera

`core/swtypes.h` separa `AccountId`, `AppId` y `ByteCount`. Preserva los 32 bits
de ambos IDs, acota `size_t` a `uint32` y combina explícitamente las dos mitades
del contexto HTTP en 64 bits. El SDK sigue definiendo los handles Steam y las
llamadas asíncronas; no se convierten en handles públicos SourceMod ni en celdas.

`core/swpawn.h` concentra la traducción de memoria, longitudes firmadas, opciones
de identidad y copia de salidas. `core/swidentity.cpp` construye `CSteamID` sin
leer `params` ni posiciones de argumentos. La única fuente añadida a AMBuilder
es ese archivo. Las firmas públicas SourcePawn no cambian.

Las rutas GameServer, stats, HTTP y GC comprueban sus traducciones antes de
invocar operaciones con esos punteros. Los buffers binarios conservan longitud
cero donde ya estaba permitida; longitudes, offsets y timeouts negativos
producen error nativo. Las opciones de universo/tipo fuera del rango del SDK y
los métodos HTTP desconocidos producen error. Las opciones cero de identidad
siguen admitidas por compatibilidad; los valores omitidos siguen siendo
universo público y cuenta individual. El cliente normal conserva la resolución
histórica de su auth string, y el fallback usa las opciones ya decodificadas.

Los resultados HTTP de tamaño, progreso y timeout usan temporales del tipo
exacto; solo se copian tras éxito. Una longitud Steam superior a `INT32_MAX`
produce error sin alterar la salida. Un booleano sobrescribe una celda completa
con 0/1. El callback de body rechaza tamaños que no permiten representar la
longitud y el terminador en Pawn antes de reservar memoria. La lectura de un
archivo comprueba `fseek`/`ftell` y su conversión antes de reservar y enviar.

Se conserva la precedencia de capacidad ausente de HTTP/GC: HTTP devuelve 0,
GC `k_EGCResultNotLoggedOn`, sin resolver direcciones. Con HTTP disponible se
valida el handle SourceMod y su acceso antes de los argumentos de la operación.
Los errores de memoria se propagan como errores nativos. Esto evita cambiar la
precedencia heredada solo para uniformar el código. Los contratos P1 permanecen
cubiertos por sus suites.

El consumidor Connect detectado en P0 llama al helper de texto con tres
argumentos, frente a cinco en la API canónica. Ahora obtiene un error controlado
antes de leer los argumentos de salida ausentes. No se añade una sobrecarga ni
se modifica ese repositorio externo. Los demás consumidores encontrados se
revisan mediante el inventario actualizado y la comparación de declaraciones.

## Registro y selección

`core/swinterfaces.h` vincula cada tipo C++ con una única versión explícita y
su header. Un `static_assert` obliga a revisar cualquier cambio de las macros
del SDK, en lugar de aprobarlo por recompilación. Se mantienen los siete nombres
de overrides existentes; no se añade un override de Client.

| Familia | Única versión admitida | Clave de gamedata |
| --- | --- | --- |
| Client | `SteamClient020` | Ninguna; fábricas con versión fija |
| GameServer | `SteamGameServer015` | `SteamGameServerInterfaceVersion` |
| Utils | `SteamUtils010` | `SteamUtilsInterfaceVersion` |
| Networking | `SteamNetworking006` | `SteamNetworkingInterfaceVersion` |
| GameServerStats | `SteamGameServerStats001` | `SteamGameServerStatsInterfaceVersion` |
| HTTP | `STEAMHTTP_INTERFACE_VERSION003` | `SteamHTTPInterfaceVersion` |
| Matchmaking | `SteamMatchMaking009` | `SteamMatchmakingVersion` |
| GameCoordinator | `SteamGameCoordinator001` | `SteamGameCoordinatorVersion` |

Las versiones corresponden a las declaraciones del SDK local 1.58a inventariado
en [P0](p0/sdk-toolchain.json). El [registro legible por máquina](p2/interface-registry.json)
identifica también métodos consumidos y pruebas. No es un catálogo de ABI
históricas ni una certificación de compatibilidad de runtimes desconocidos.

Sin override se usa la versión registrada. Un override idéntico es válido;
uno vacío, de otra familia o desconocido desactiva esa familia antes de llamar
a fábricas, getters o resolver usuario/pipe. Se registra una sola razón con
familia, versión pedida y esperada. El rechazo permanece hasta `Reset`, aunque
cambie el valor en memoria. Las adquisiciones exitosas se almacenan hasta ese
mismo reset. Una interfaz de versión conocida que devuelve null no queda
inhabilitada: puede reintentarse cuando aparezca el runtime.

La ruta global preexistente `g_pSteamClientGameServer` conserva su prioridad.
Su declaración sigue siendo la del SDK compilado; estas pruebas no identifican
la versión efectiva del objeto global del runtime. Las dos fábricas alternativas
usan el literal `SteamClient020`; no prueban versiones anteriores a ciegas.
La adquisición GC mediante `void*` solo se convierte después de validar la versión.
Las rutas internas y firmas de gamedata existentes no se presentan como perfiles
binarios aprobados: esa evidencia corresponde a P4.

## Pruebas y evidencia

```bash
bash scripts/test-typed-boundary.sh
bash scripts/test-stats-natives.sh
bash scripts/test-detours.sh
bash scripts/test-public-ip.sh
AMTL_DIR=<sourcemod>/public/amtl bash scripts/test-steamid-string.sh
```

La nueva suite se añade al job Linux x86 de CI, sin ejecutar CI remoto. Las
pruebas locales usan GCC 13 x86_64 y GCC 12 x86 con UBSan, SDK real y servicios
simulados. El contenedor multilib existente se ejecuta sin red y con el repo
solo lectura, salvo `.build`.

- 184 comprobaciones del resolver real para las siete familias: defaults,
  coincidencia exacta, cache, versiones negativas, ausencia de llamadas antes
  del rechazo, diagnóstico único, reset y reintento de disponibilidad.
- 46 comprobaciones de frontera/identidad en x86_64 y 45 en x86. La diferencia
  es el caso `size_t > UINT32_MAX`, imposible de representar en x86.
- 57 comprobaciones HTTP/GC: memoria inválida, longitudes, enums, salida en
  fallo incluso si Steam escribe el temporal, canarios, bool, contexto, llamada
  asíncrona de 64 bits, handle y capacidad ausente.
- Dos compilaciones que deben fallar: cambio de versión SDK y mezcla de
  `AppId` con `AccountId`.
- Regresión P1: 32 + 34 + 26 + 30 por cada signo de char + 19 escenarios por
  arquitectura. Los dobles adaptan la firma interna de identidad; siguen usando
  los mismos entrypoints y expectativas públicas anteriores.
- Controles negativos sobre la copia local anterior a P2: el resolver acepta
  una versión desconocida y HTTP altera una salida al fallar. Ambos ejecutables
  fallan como se esperaba con las nuevas expectativas; los actuales pasan.
- Comparación con P0 de declaraciones, enums, callbacks, registros, binding y
  lista opcional, ignorando números de línea: iguales. El include público no
  se ha modificado en P2.
- Harness SourcePawn recompilado con 1.12.0.7259, sin warnings; no equivale a
  ejecutar los consumidores en servidor.
- Siete objetos ELF32 compilados contra SDK, SourceMod y HL2SDK reales:
  `swgameserver`, `swidentity`, `ssnatives`, `gsnatives`, `gcnatives`,
  `swhttprequest`, `extension`. Se omite únicamente el flag C preexistente
  `-Wno-pointer-to-int-cast` en estos comandos diagnósticos; `-Werror` se conserva.
  Esto no equivale a un build enlazado de la extensión.

Resultados y hashes: [validation-results.json](p2/validation-results.json).
Logs, comandos de objetos, controles y escaneo actualizado: `.build/p2/` local.
Los resultados P0/P1 publicados en este árbol son capturas históricas; sus hashes
no se reescriben para describir P2.

## Límites pendientes

T10-U, T24-B y las partes unitarias de T28/T29/T30 tienen evidencia local para
el alcance descrito. T11-I/R sigue pendiente: el registro y los dobles no
certifican las capacidades de un runtime real. T25-I también sigue pendiente:
comparar fuentes y recompilar el harness no demuestra carga ni ejecución de
consumidores remotos o plugins precompilados. No se valida una vtable real, alineación de todas las
estructuras entre plataformas, callbacks de Steam en ejecución ni Windows.
No hay aceptación SRCDS del binario modificado ni build completo enlazado nuevo.
Las direcciones base se validan con SourceMod; el llamador sigue siendo
responsable de que la capacidad positiva describa su buffer completo. No se
implementa un validador general del rango de memoria de la VM.

El ciclo de vida completo, callbacks tardíos y propiedad de recursos siguen
para P3; las firmas internas y perfiles binarios para P4. La selección tipada
no corrige por sí sola esas obligaciones.
