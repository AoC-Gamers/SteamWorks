# P5-R5: integración y seguimientos de aceptación

Rama `develop`, base `46e3ee05fc29ad090a1d6f7b1cb5e3dca9855350`. La preparación
local renovó validación, documentación, artefactos y reversión sin intervenir
servidores; los builders son efímeros y sin red. El seguimiento autorizado de
reversión posterior operó únicamente sobre el laboratorio DEV y restauró el
servidor habitual. Sin commit, publicación ni producción.
**P5 sigue sin aceptación global**. La entrega es exclusivamente Linux i386.

## Evidencia conciliada

La reconstrucción estricta produce el mismo binario aceptado:
`bc76b00d0614f8f8489f8d3d30e3d654976f3bfde689f17d8731018d6c7ac639`.
Las aceptaciones directas de ese payload conservan estos alcances:

| Evidencia | Resultado observado en SRCDS x86 |
|---|---|
| [HTTP/streaming](http-stream-srcds-acceptance.md) | 32 completions, cuatro respuestas y 24 bytes streaming, contexto/payload correctos, mapa, cancelación y cierre |
| [Autenticación/identidad/stats P5-R3](p5-r3-auth-srcds-acceptance.md) | Payload del ZIP, un humano autorizado, un evento 014 y uno por forward público, identidad/buffers/licencia correctos; stats enteros iguales por ambas rutas y fallo float preservado |
| [Floats y claves inexistentes P5-R4](p5-r4-stats-srcds-acceptance.md) | Dos recorridos de 733 claves; las mismas 12 claves float finitas con bits iguales por cliente/AccountID; 16 lecturas de clave inexistente rechazadas por los cuatro getters conservando salidas |

Los ensayos usaron LinuxGSM, un árbol SourceMod aislado en 27015 y el perfil
`l4d2-23990100-linux-x86`. Terminaron con cierre explícito, restauración del DEV
habitual, hashes originales y conexión a Steam verificados. No hubo logs de error
SourceMod ni marcadores de crash en las fases registradas. La VM instalada no se
reemplazó y SRCDS no estuvo instrumentado con ASan.

La [aceptación anterior del adaptador 014](auth014-srcds-acceptance.md) pertenece
al binario `ddfdf9bf…` y mantiene su atribución histórica. No se usa para inferir
la autenticación del binario final: esta se ejecutó directamente. No se
inspeccionaron tickets ni AccountIDs del forward ni se escribieron stats.

T03 ya no tiene pendientes de float positivo ni clave inexistente. La prueba
esperó un control entero válido antes de comprobar fallos, usó sentinels enteros
y NaN distintos y repitió la clave ausente antes/después de cada recorrido. Un
float «positivo» significa lectura exitosa, finita y coincidente, no un valor
numérico mayor que cero. Las otras 721 claves rechazaron float preservando sus
salidas; no se clasifican automáticamente como enteras ni como claves inexistentes.

Quedan aceptados en el alcance descrito: autenticación/validación, identidad del
humano, lectura entera y float por ambas rutas, preservación en tipo incorrecto
previo y clave inexistente, HTTP/streaming, arranque/mapa/quit y carga del payload
desde ZIP P5-R3/R4. El ZIP P5-R5 tiene runtime idéntico y documentación renovada;
se valida localmente y no se instala de nuevo para atribuirle otra aceptación.

## Seguimiento de reversión en sitio

La [aceptación posterior del ZIP de reversión](p5-r5-rollback-srcds-acceptance.md)
restaura extensión/include/gamedata en el mismo árbol aislado y arranca el baseline
real con LinuxGSM. Hashes, configuración ajena, módulos mapeados, Steam y HTTP
pasan; T35 queda aceptado en ese alcance.

Se observó además `SIGSEGV` al salir tanto del candidato como del baseline, tras
sus mensajes de cierre. Los binarios originales no acreditan terminación limpia: T08 registró el fallo
original, con cores filtrados. El [seguimiento T08](t08-srcds-shutdown.md) identifica
la causa y valida tres cierres limpios de un nuevo candidato Linux x86. La restauración/arranque y el cierre
son resultados distintos. El DEV habitual fue restaurado. Los ZIP previos no se
regeneran; la [matriz anterior](p5/history/p5-r5-before-rollback-acceptance-validation-results.json)
se conserva y el seguimiento tiene evidencia separada.

## Validación local repetida

- Build L4D2 x86 con warnings como errores; SDK 1.58a y 53 headers/dependencias
  contrastados con P0. Hash idéntico al runtime aceptado y enlace portable de
  `libtier0_srv.so`, sin redistribuir módulos Steam/engine.
- Trece suites x86 pasan y comparan la API con P0: declaraciones, registros,
  enums, typesets y opcionalidad intactos.
- 18 comprobaciones SourceHook real del enlace 014 y control negativo 015;
  92 comprobaciones streaming con fixture binaria independiente y control negativo.
- 227 comprobaciones CForward/VM y 16 ciclos de entorno. La VM x86 instrumentada
  fijada se reutiliza como diagnóstico, sin instalación en servidor. ASan/LSan/UBSan
  permanecen activos sin supresiones, también en lifecycle nativo y streaming.
- Doce pruebas de empaquetado/reversión; extracción/validación del candidato y
  roundtrip de los ZIP reales en filesystem temporal, conservando una config ajena
  y eliminando gamedata originalmente ausente.
- Tres consumidores recompilados sin warnings y cuatro tablas de imports válidas.
  El harness nuevo de stats también recompila sin warnings; el generador produce
  el mismo fixture de 733 claves que se usó en la aceptación.
- Inventario renovado: 2679 archivos examinados, 27 con referencias; ocho son
  fixtures/copias de harness y permanecen los 19 archivos de producto. Las dos
  llamadas HTTP de Connect con tres argumentos donde se requieren cinco siguen
  identificadas y requieren corrección externa.
- actionlint, sintaxis shell, hashes de evidencias/paquetes anteriores y enlaces
  documentales comprobados. CI remota no ejecutada; sin nuevas pruebas en SRCDS.

La [matriz T01–T36](p5/validation-results.json) conserva niveles ejecutados y
pendientes: T03 incorpora el resultado real, T15/T29 mantienen negativos de
identidad y T35 incorpora la reversión real desde ZIP. T08 registra el fallo comparativo
de salida observado después del checkpoint local. [Logs actuales](p5/r5/integration.json), [inventario](p5/api-inventory.json)
y [consumidores](p5/consumers.json) acompañan los informes de runtime originales.

## Artefactos y reversión

Conjunto nuevo `.build/p5-r5/review/`, con manifiesto, verificación y `SHA256SUMS`:

| ZIP | Contenido |
|---|---|
| `steamworks-p5-r5-linux-x86-candidate.zip` | Extensión ELF32 aceptada, include, licencia y documentación conciliada |
| `steamworks-p5-r5-competitive-dev-rollback.zip` | Binario/include originales renovados por lectura; gamedata ausente registrada |
| `steamworks-p5-r5-source-review.zip` | Fuentes propias, scripts, pruebas, parches y diff contra HEAD; sin SDK/deps |
| `steamworks-p5-r5-vm-diagnostics.zip` | Solo VM y receptores sintéticos x86 con licencia/procedencia; fuera de `addons` |

La captura de reversión se renueva con `docker cp`, sin operaciones de procesos,
y conserva los hashes anteriores. La [reversión](p5-rollback.md) diferencia el
roundtrip de archivos, las restauraciones reales del selector aislado y la
reversión en sitio del ZIP, aceptada después con el incidente de cierre separado. No se incluyen CFG privados,
datos, tickets, módulos Steam ni VM en el payload instalable. Los ZIP/checksums
anteriores y los informes de aceptación permanecen intactos.

El [checkpoint P5-R4 aceptado](p5/history/p5-r4-accepted-validation-results.json)
y sus documentos se preservan en `p5/history/`. Los diagnósticos x64 antiguos son
historia: no se repitieron ni se distribuyen en esta entrega.

## Reproducir sin desplegar

Seleccionar dependencias fijadas, `-m32`, `SOURCEMOD_DIR`, `MMSOURCE_DIR`,
`AMTL_DIR`, `SPCOMP` y la VM x86 instrumentada. Las suites VM usan
`VM_TEST_CXX=clang++`, `VM_TEST_FLAGS` con `-m32 -fsanitize=address -shared-libasan
-fsized-deallocation -fno-pie -no-pie` y el runtime ASan registrado, sin supresiones.

```sh
python3 scripts/test-integration.py --with-auth-binding --with-auth-forward \
  --with-sourcepawn-environment --output .build/mi-integracion-x86
python3 scripts/stage-artifact.py . .build/p5-r5/build/package .build/mi-candidato
EXTS_ARTIFACT_DIR="$PWD/.build/mi-candidato" bash scripts/ci-validate-artifact.sh
python3 scripts/prepare-review-artifacts.py --candidate .build/mi-candidato \
  --baseline .build/p5-r5/baseline --output .build/mi-entrega --label mi-checkpoint
```

Cada salida debe ser nueva. Las recetas complementarias van en el ZIP de fuentes,
bajo `review/`. Este flujo no instala el candidato ni reemplaza la VM del servidor.

## Pendientes vigentes

- Negativos de identidad en host real y matriz completa de consumidores/forwards.
- GC, ejercicio completo de interfaces y negativos de integración todavía
  cubiertos solo con proveedores simulados.
- Carga tardía/unload en caliente, owner unload con petición pendiente y carreras
  de generaciones antiguas más amplias que la respuesta retenida de T08.
- Desconexión controlada de Steam sin destruir el runtime y operación sostenida
  con medición de recursos/logs.
- Regenerar una entrega P5 integrada con el nuevo ELF cuya [aceptación T08](t08-coexist-srcds-acceptance.md)
  pasa reinicialización, HTTP retenido y quit con SourceTVSupport. El paquete
  P5-R5 conserva su payload anterior.
- Corrección externa de las dos llamadas HTTP de Connect. Windows y otras
  parejas de módulos quedan fuera del soporte de esta entrega.
