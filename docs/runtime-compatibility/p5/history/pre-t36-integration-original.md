# P5: integración y candidato local

Este informe y sus artefactos describen el checkpoint previo a
[T36](t36-auth-forward.md). La corrección posterior tiene su evidencia separada;
los resultados y ZIP históricos de P5 no se regeneran ni certifican el nuevo binario.

Fecha: 2026-11-14. Rama `develop`, base
`46e3ee05fc29ad090a1d6f7b1cb5e3dca9855350`, árbol con cambios sin commit.
Resultado: validación local integrada y artefactos preparados; aceptación
operativa global pendiente. No se desplegó, publicó ni operó SRCDS en P5.

## Cambios y revisión

P1–P4 se integran manteniendo el contrato público. P5 añade un ejecutor de suites
con control automático del inventario P0, preparación local de candidato y
reversión con manifiestos, y documentación consolidada. El paquete ahora incluye
`LICENSE`, comprobada también por el validador. El plugin de pruebas sigue fuera
del paquete instalable.

La [arquitectura](architecture.md), el [registro de compatibilidad](compatibility.md)
y la [reversión](p5-rollback.md) describen el estado final. H01/H02/H03/H05/H08/H09
tienen regresiones deterministas; H04 se limita al perfil probado P4; H06 se
resuelve retirando helpers sin consumidores; H07 documenta gamedata opcional
ausente. H10 incorpora ownership, generaciones y desmontaje, probado con dobles
y parcialmente en SRCDS P4; operaciones remotas y carga sostenida siguen pendientes.

## Validación

- Ocho suites integradas en host y x86: siete de código/contratos y nueve casos
  de empaquetado/reversión. UBSan en los harnesses C++; el decoder x86 se prueba
  sin instrumentación. Los logs distinguen casos y comprobaciones, sin sumar
  ejecuciones repetidas como cobertura diferente.
- ASan con detección de fugas sobre lifecycle en host: sin errores observados.
- Build completo Linux x86 con warnings como errores, SDK 1.58a y dependencias
  fijadas por revisión en la imagen registrada en P0. Los 53 headers y librerías
  de enlace se contrastan con sus hashes P0. No se usa una actualización de SDK.
- Compilación sin warnings del harness, AoC y Family Share con SourcePawn
  1.12.0.7259. Imports registrados presentes en tres SMX recompilados y un SMX
  precompilado; no equivale a carga o ejecución de sus operaciones.
- Inventario de declaraciones, registros, enums, callbacks y opcionalidad igual
  a P0. Corpus de consumidores y discrepancias HTTP conservados.
- Paquete extraído validado, ELF32 identificado, contenido y hashes comparados,
  sin SDK/configs/plugins de prueba. Reversión del ZIP probada en filesystem.
- Workflow revisado con actionlint, `git diff --check` limpio. CI remota no ejecutada.

Resultados por T01–T36, tiempos medidos, entorno y hashes:
[validation-results.json](p5/validation-results.json). Los informes anteriores
son capturas históricas; no se reescriben para atribuirles pruebas posteriores.

## Artefactos locales

Directorio `.build/p5/review/`:

- `steamworks-p5-linux-x86-candidate.zip`: extensión, include, licencia y docs;
- `steamworks-p5-competitive-dev-rollback.zip`: conjunto anterior capturado;
- `steamworks-p5-source-review.zip`: snapshot de fuentes propias para revisión;
- `manifest.json`: hashes de payload y ZIP, ausencia de gamedata y alcance;
- `SHA256SUMS`: comprobación externa de archivos de la entrega.

La extensión candidata tiene SHA-256
`28c34d3c5c1599f00c532037516c66aa7a6c0bdcb62294e33cad1083094e5632`.
Coincide byte por byte con la extensión ejercitada en P4. El snapshot incluye
fuentes propias y scripts, sin SDK/deps ni documentos privados del pedido. No
representa un commit ni incluye binarios de consumidores externos.

## Reproducir sin desplegar

Con SDK y AMTL disponibles, seleccionar una salida nueva en cada ejecución:

```bash
AMTL_DIR=/ruta/sourcemod/public/amtl \
  python3 scripts/test-integration.py --output .build/mi-validacion-host
TEST_CXXFLAGS=-m32 AMTL_DIR=/ruta/sourcemod/public/amtl \
  python3 scripts/test-integration.py --output .build/mi-validacion-x86
python3 scripts/stage-artifact.py . .build/p5/build/package .build/mi-candidato
EXTS_ARTIFACT_DIR="$PWD/.build/mi-candidato" bash scripts/ci-validate-artifact.sh
python3 scripts/prepare-review-artifacts.py --candidate .build/mi-candidato \
  --baseline .build/p5/baseline --output .build/mi-entrega
```

El baseline es una captura previa, no una ruta del servidor. El preparador
rechaza sobrescribir una entrega existente. La imagen y revisiones exactas del
build local están en los resultados. Los scripts generales de adquisición y CI
todavía siguen ramas; no constituyen por sí solos un lock reproducible de P5.

## Puertas abiertas

No se propone despliegue todavía. Faltan las pruebas I/R actuales de interfaces
y contratos remotos HTTP/TLS/timeout/stats/GC, carga de consumidores existentes,
desconexión sin destruir runtime y operación sostenida. T36 requiere resolver
el contrato binario del forward de autenticación; no se declara cubierto por
los tests de ownership. T35-I requiere reversión con arranque real autorizado.
Windows está fuera del alcance de soporte de este candidato. Estas limitaciones
impiden declarar toda la matriz aprobada aunque el build y las pruebas U pasen.
