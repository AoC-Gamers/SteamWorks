# P5-R4: integración de la aceptación final de P5-R3

Rama `develop`, base `46e3ee05fc29ad090a1d6f7b1cb5e3dca9855350`. P5-R4 renueva
validación local, documentación, artefactos y reversión, sin commit ni despliegue
del candidato. Se encendió `competitive_dev` por solicitud del operador, con su
configuración habitual; eso no es una instalación del candidato. **P5 sigue sin
aceptación global**. La entrega es exclusivamente Linux i386.

## Aceptaciones incorporadas

La reconstrucción estricta produce exactamente el binario ya aceptado:
`bc76b00d0614f8f8489f8d3d30e3d654976f3bfde689f17d8731018d6c7ac639`.
Las pruebas reales corresponden a ese payload, con estos alcances separados:

| Evidencia | Resultado observado en SRCDS x86 |
|---|---|
| [HTTP/streaming](http-stream-srcds-acceptance.md) | 32 completions, cuatro respuestas y 24 bytes streaming, ambas celdas de contexto/payload correctos, mapa, cancelación y cierre |
| [Autenticación/identidad/stats de P5-R3](p5-r3-auth-srcds-acceptance.md) | Payload extraído del ZIP revisado; un humano autorizado, un evento 014 y uno en cada forward público, identidad/buffers/licencia correctos; stats enteros iguales por cliente/AccountID y fallo float con sentinels preservados |

Ambos ensayos usaron LinuxGSM, un árbol aislado en 27015 y el perfil
`l4d2-23990100-linux-x86`; terminaron con cierre explícito, restauración del DEV
habitual, hashes originales y conexión a Steam comprobados. No hubo logs de
error SourceMod ni marcadores de crash en las fases registradas. La VM instalada
no se reemplazó y SRCDS no estuvo instrumentado con ASan.

La aceptación anterior del [adaptador 014](auth014-srcds-acceptance.md), binario
`ddfdf9bf…`, conserva su atribución histórica. Ya no hace falta inferir de ella la
entrada de un jugador con el binario final: esta fue probada directamente en
P5-R3. No se inspeccionaron tickets ni AccountIDs del forward, ni se escribieron
stats. La semántica binaria mantiene sus pruebas sintéticas con CForward/VM.

Quedan cerrados en su alcance observado: autenticación y ambos forwards públicos,
identidad/buffers del humano, solicitudes y lectura entera por ambas rutas,
preservación de salidas en fallo float, HTTP/streaming, arranque/mapa/quit y carga
del payload runtime desde el ZIP P5-R3. Esto no aprueba todas las variantes de cada
capacidad ni una instalación del ZIP P5-R4: su payload runtime es idéntico y su
contenido documental se valida localmente.

La [aceptación posterior de float y claves inexistentes](p5-r4-stats-srcds-acceptance.md)
valida directamente el mismo payload P5-R4 con una cuenta autorizada: dos rondas,
12 claves float finitas con bits iguales por cliente/AccountID y clave inexistente
rechazada por los cuatro getters conservando las salidas. T03 queda aprobado para
ese alcance. La [matriz previa](p5/history/p5-r4-before-float-stats-validation-results.json)
y los ZIP de P5-R4 se conservan; este seguimiento tiene evidencia separada.

## Validación repetida de P5-R4

- Build L4D2 x86 con warnings como errores, SDK 1.58a y 53 headers/dependencias
  contrastados contra P0. Hash idéntico al candidato aceptado; enlace portable
  `libtier0_srv.so`, sin redistribuir SDK ni módulos Steam/engine.
- Trece suites x86 pasan y comparan la API pública con P0: declaraciones,
  registros, enums, typesets y opcionalidad sin cambios.
- 18 comprobaciones SourceHook real del enlace 014 y control negativo 015;
  92 comprobaciones de contexto streaming con fixture binaria independiente y
  control negativo del comportamiento anterior.
- 227 comprobaciones CForward/VM y 16 ciclos de propiedad de entorno. VM x86
  instrumentada de procedencia fijada, reutilizada como diagnóstico separado.
  ASan/LSan/UBSan activos sin supresiones, también en lifecycle nativo y streaming.
- Doce pruebas de empaquetado/reversión; extracción/validación del candidato y
  restauración desde los ZIP reales en filesystem temporal, preservando una
  configuración ajena y retirando gamedata originalmente ausente.
- Tres consumidores recompilados sin warnings y cuatro tablas de imports válidas.
  Su semántica completa en host real sigue pendiente.
- Inventario actualizado: 2676 archivos examinados, 25 con referencias; seis son
  fixtures/copias de harness y permanecen los 19 archivos de producto. Persisten
  las dos llamadas HTTP de Connect con tres argumentos donde se requieren cinco.
- actionlint, sintaxis shell, checksums históricos y enlaces documentales
  comprobados. CI remota no ejecutada. No se repitieron aceptaciones en SRCDS.

La [matriz T01–T36](p5/validation-results.json) distingue casos aprobados, parciales
y pendientes. T01/T02/T36 incluyen la aceptación directa del candidato final;
T03 incorpora la aceptación float/missing-key; T15/T29 mantienen sus variantes pendientes; T26 diferencia el payload P5-R3
instalado del ZIP nuevo; T35 no confunde restauración de selector con reversión
en sitio desde un archivo. [Logs actuales](p5/r4/integration.json),
[inventario](p5/api-inventory.json) y [consumidores](p5/consumers.json) acompañan
las evidencias originales, que permanecen intactas.

## Artefactos y reversión

Conjunto nuevo en `.build/p5-r4/review/`, con `manifest.json`, verificación y
`SHA256SUMS`:

| ZIP | Contenido |
|---|---|
| `steamworks-p5-r4-linux-x86-candidate.zip` | Extensión ELF32 aceptada, include, licencia y documentación conciliada |
| `steamworks-p5-r4-competitive-dev-rollback.zip` | Baseline renovado por lectura; binario/include originales y ausencia de gamedata registrada |
| `steamworks-p5-r4-source-review.zip` | Fuentes propias, pruebas, scripts, parches y diff contra HEAD; sin SDK/deps |
| `steamworks-p5-r4-vm-diagnostics.zip` | Solo VM/receptores sintéticos x86 con licencias/procedencia; fuera de `addons` |

La captura de reversión se leyó del contenedor detenido mediante `docker cp`, sin
iniciarlo para capturar archivos; coincide con los hashes anteriores. El arranque
posterior del contenedor fue una solicitud independiente del operador.
La [reversión](p5-rollback.md) conserva la distinción entre roundtrip de archivos,
restauraciones reales del selector aislado y reversión en sitio del ZIP pendiente.
No se incluyen CFG privados, datos, tickets, módulos Steam ni VM en el payload
instalable. Los ZIP anteriores y sus checksums se mantienen sin sobrescribir.

El [checkpoint P5-R3 aceptado](p5/history/p5-r3-accepted-validation-results.json)
y sus documentos se conservan en `p5/history/`. El diagnóstico x64 antiguo es
historia; no se repitió ni se entrega en este conjunto.

## Reproducción local

Seleccionar dependencias fijadas, `-m32`, `SOURCEMOD_DIR`, `MMSOURCE_DIR`,
`AMTL_DIR`, `SPCOMP` y la VM x86 instrumentada. Para las suites VM, usar
`VM_TEST_CXX=clang++`, `VM_TEST_FLAGS` con `-m32 -fsanitize=address -shared-libasan
-fsized-deallocation -fno-pie -no-pie` y el runtime ASan registrado. Mantener las
opciones ASan/LSan sin supresiones.

```sh
python3 scripts/test-integration.py --with-auth-binding --with-auth-forward \
  --with-sourcepawn-environment --output .build/mi-integracion-x86
python3 scripts/stage-artifact.py . .build/p5-r4/build/package .build/mi-candidato
EXTS_ARTIFACT_DIR="$PWD/.build/mi-candidato" bash scripts/ci-validate-artifact.sh
python3 scripts/prepare-review-artifacts.py --candidate .build/mi-candidato \
  --baseline .build/p5-r4/baseline --output .build/mi-entrega --label mi-checkpoint
```

Cada salida debe ser nueva. Las recetas complementarias van en el ZIP de fuentes,
bajo `review/`. Ninguna instala el candidato ni reemplaza la VM del servidor.

## Pendientes vigentes

- Negativos de identidad en host real y matriz completa de consumidores/forwards.
  Los floats positivos y las claves inexistentes ya pasaron para el caso descrito.
- GC, ejercicio completo de interfaces y negativos de integración todavía
  cubiertos solo con proveedores simulados.
- Reinicialización en el mismo proceso, carga tardía/unload en caliente, owner
  unload con petición pendiente y generaciones antiguas en host real.
- Desconexión controlada de Steam sin destruir el runtime y operación sostenida
  con medición de recursos/logs.
- Reversión en sitio desde el ZIP con arranque posterior. La carga del payload
  P5-R3 y las restauraciones del selector aislado ya pasaron; son pruebas distintas.
- Corrección externa de las dos llamadas HTTP de Connect. Windows y otras
  parejas de módulos quedan fuera del soporte de esta entrega.
