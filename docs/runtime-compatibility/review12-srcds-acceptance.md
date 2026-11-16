# Revisión 12: buffers GC y HTTP en SRCDS x86

2026-11-16. **Aceptación del alcance probado**, sobre el ELF optimizado
`72664c658f6d28e78a975ec98c96879dc641f96725206f514f159ea17125fcb3`.
REV-HTTP-02 pasa contra el proveedor Steam HTTP real. REV-GC-01 pasa para la cola
vacía real y para cinco casos de un proveedor local tipado, con el hook del
candidato y la VM SourcePawn reales. **No se recibió un mensaje GC remoto**;
la rama positiva del proveedor Steam real sigue sin acreditarse.

Rama `develop`, base `a0016a23adc70dd9c1bdf2bf22bf8ae0e685dbc6`, sin commit,
push ni producción. Las fuentes nativas y el include público son exactamente
los de la [corrección local](review12-buffers.md); no se añade una API de producto.
Los natives del fixture son exclusivamente de laboratorio y no se empaquetan
con SteamWorks. No se transfiere la aceptación histórica P5-R18 al nuevo ELF.

## Aislamiento y verificación del candidato

Se usó `competitive_dev`, `/app/l4d2server` mediante LinuxGSM, puerto 27015 y
selector temporal `addons/sourcemod_acceptance`. Se conservaron los flags y CFG
habituales, sin añadir contraseña. El servidor habitual se detuvo con LinuxGSM
antes de copiar. Solo en el laboratorio se desactivó la actualización automática
de SourceMod y se omitieron los plugins de producto; se mantuvieron las
extensiones habituales, incluida SourceTVSupport. El único plugin nuevo fue
`steamworks_buffer_acceptance.smx`, con su extensión auxiliar.

La configuración original permaneció en un recovery privado durante el ensayo.
Se protegieron mediante SHA-256 el selector LinuxGSM, el ELF habitual y las
configuraciones Family Share/AoC. No se exportaron secretos, argv, entorno,
identidades, tickets ni logs completos. El fixture HTTP escuchó exclusivamente
en `127.0.0.1:27880`; no dependió de una cuenta humana ni una clave Web API.

Se verificaron SHA/inode del ELF realmente mapeado y su formato ELF32/i386. Los
RVA proceden de símbolos y desensamblado de ese ELF exacto: getter tipado
`GetGameCoordinator`, slot del resolver y, para el proveedor local, slot de
`pGCHooks` y método C++ `AddHooks` (no es API SourcePawn pública). Se comprobaron rangos de memoria
escribibles/ejecutables antes de llamar. No se adivinó una ABI ni se escribió un
parche de instrucciones desde el harness.

El fixture auxiliar se compiló con g++ x86 del builder fijado y el harness con
SourcePawn **1.12.0.7259**, sin warnings. Esta versión se leyó del compilador
presente; no se reutilizó la etiqueta 7246 de compilaciones históricas.
La VM y las bibliotecas Steam/engine instaladas no se sustituyeron.

## REV-HTTP-02: proveedor Steam real

Dos rondas, antes y después de `map c2m2_fairgrounds`, con tres respuestas:

| Respuesta loopback | Estado | Tamaño | Capacidades por respuesta |
|---|---:|---:|---|
| `/text`, `steamworks-local` | 200 | 16 | 1, 2, 4, 16, 17, 32 |
| `/empty` | 200 | 0 | 1, 2, 4, 16, 17, 32 |
| `/error`, `fixture-error` | 503 | 13 | 1, 2, 4, 16, 17, 32 |

**36 combinaciones**, seis completions únicas y contexto de dos celdas correcto.
En cada lectura se comprobaron resultado, prefijo exacto, byte NUL, `written`,
`truncated` y todos los canarios restantes de un buffer de 40 bytes. El caso
16→16 exige 15 bytes + NUL; 16→17 conserva el cuerpo completo sin truncamiento;
capacidad 1 publica solo NUL. El cuerpo vacío también pasa sin truncamiento.
HTTP 503 conserva su cuerpo disponible: no equivale a un fallo de la API lectora.

Antes de enviar cada petición se intentó leer el texto. Las seis consultas
fallidas conservaron destino, `written` y `truncated`. Se cerraron los handles
dentro del callback y se difirió la siguiente petición mediante un timer.
Los fallos de reserva, de lectura completa con escritura parcial del proveedor,
el rango extremo y la aridad antigua siguen respaldados por las pruebas x86
locales; **no se simularon como fallos reales del SDK ni se forzó OOM en SRCDS**.
No se atribuye esta prueba a streaming, que tiene otro contrato.

## REV-GC-01: interfaz real y proveedor local separados

### Cola vacía del GC real

Cuatro rondas de veinte consultas, dos antes y dos después del nuevo juego:
capacidades 0, 1, 3, 16 y 4096, con retornos de consumidor 0, -1, 2 y 4.
`IsMessageAvailable` confirmó cola vacía antes de cada consulta. Si hubiera
mensajes reales, el fixture habría omitido la recepción y marcado el caso
pendiente para no consumirlos; no ocurrió.

Las **80 llamadas** atravesaron el hook del candidato y emitieron exactamente
un forward cada una. El consumidor verificó tipo/tamaño cero y todos los bytes
anunciados inicializados a cero. El buffer nativo de origen y sus canarios
permanecieron con `0xa5`, demostrando que el snapshot no expone ni borra su cola.
El proveedor Steam observado escribió tipo y tamaño a cero al fallar; esta
observación no sustituye el test ASan local que prohíbe leer outputs sin escribir.

Los retornos positivos 2/4 se propagaron. Un consumidor con retorno -1 se agrega
a cero por el `ET_Event` real y conserva `NoMessage`, igual que retorno 0.
No se cambió esa semántica histórica ni se afirmó que -1 atraviese intacto la
agregación real hasta el hook.

### Proveedor local tipado en el mismo proceso

Un objeto separado implementó exactamente `ISteamGameCoordinator` del SDK
1.58a, sin registrarse con Steam ni enviar mensajes. El helper invocó **la entrada
`AddHooks` del candidato**, pasando ese objeto, y luego realizó una llamada
virtual para atravesar SourceHook, la implementación de producción y la VM real.
Restauró el GC real mediante `AddHooks` antes de regresar a Pawn o al game frame.
No creó forwards alternativos ni sustituyó el resolver de interfaces.

| Caso local | Capacidad | Bytes válidos / resultado | Verificado |
|---|---:|---|---|
| Mensaje corto binario | 4096 | `00 80 ff`, OK | Prefijo intacto, cola completa cero, tipo con bit alto |
| Mensaje binario completo | 16 | `80`…`8f`, OK | Los 16 bytes sin exigir NUL |
| Mensaje vacío | 0 | 0, OK | Tipo preservado, longitud/tamaño cero |
| Outputs no escritos | 16 | NoMessage | Metadatos y snapshot cero, outputs del proveedor intactos |
| Tamaño requerido mayor | 16 | BufferTooSmall, requerido 17 | Forward tamaño cero, buffer y outputs del proveedor intactos |

Cinco casos por ronda, **10 llamadas locales**, una llamada original y un
callback VM por caso. Se comprobaron bytes y canarios del proveedor después del
forward, sin copyback. Las rondas reales posteriores verificaron que el GC real
volvía a tener sus hooks funcionales. Estos casos acreditan el marshalling de la
corrección en SRCDS, **no una entrega de contenido desde el backend Steam GC**.

## Resultado, cierre y restauración

Las rondas finales acumulan **372 comprobaciones y cero fallos**. El PID
**35376**, con ticks iniciales **1641953**, permaneció igual durante el nuevo
juego, HTTP, proveedor local y restauraciones GC. No hubo logs de error SourceMod
ni marcadores de crash. El cierre del candidato mediante `quit` fue limpio;
también terminó limpio el primer arranque preparatorio con el mismo ELF.

El DEV habitual se restauró mediante LinuxGSM, PID **37450**, conectado a Steam,
con sus cuatro hashes protegidos idénticos. Se retiraron laboratorio, recovery
y fixture HTTP. No se borraron logs ni cores originales. El primer sondeo de
restauración agotó su ventana de 20 segundos antes de la conexión Steam del DEV;
se conservó recovery y se repitieron exactamente las comprobaciones cuando
estuvo listo, sin cambiar CFG ni reiniciar de nuevo.

El primer verificador de evidencia confundió el nombre de un check HTTP exitoso
(`HTTP_pending_outputs_unchanged`) con un marcador GC pendiente por buscar
`pending_` de forma demasiado amplia. Se corrigió solo ese clasificador; no se
descartó ninguna traza ni se suprimieron checks, errores o comprobaciones de crash.

La API y las fuentes de las dos correcciones siguen coincidiendo byte por byte
con la revisión local; se verificaron además **155 archivos de artefactos
históricos**, el documento original de OneDrive y la matriz P5-R18 intactos.
No se repite aquí la aceptación global de autenticación/stats, todos los
consumidores, Windows, fugas del proceso ni CI remoto del worktree sin commit.
Queda pendiente un escenario autorizado de **recepción positiva GC real**.

## Evidencia reproducible

[Resultados, hashes y límites](review12-srcds/results.json),
[checks finales](review12-srcds/checks.json),
[HTTP inicial](review12-srcds/HTTP-final.json),
[HTTP tras mapa](review12-srcds/HTTP-after-map.json),
[GC real](review12-srcds/GC-final.json),
[GC local](review12-srcds/GC-local-final.json),
[GC real restaurado](review12-srcds/GC-restored-real.json),
[módulos finales](review12-srcds/final-modules.json),
[salida](review12-srcds/shutdown.json) y
[restauración DEV](review12-srcds/cleanup.json).

Se conservan las fuentes en `tests/runtime/steamworks_buffer_*` y el build en
`scripts/build-buffer-fixture.sh`. Los scripts de operación y el perfil del ELF
se adjuntan como evidencia del ensayo, ligados a este hash y a esta instalación;
no son un instalador general ni deben ejecutarse contra producción.
El paquete separado es
`.build/review12-srcds/review/steamworks-review12-linux-x86-srcds-acceptance.zip`.
Los ZIP de la revisión local y de P5-R18 permanecen intactos.
