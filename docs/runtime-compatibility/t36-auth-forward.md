# T36: marshalling binario de autenticación

La [revisión posterior de la ruta del engine y la convar de debug](auth-debug.md)
identifica objetos distintos para SteamGameServer014/015. Sus resultados no
cambian la aceptación determinista de este documento ni aprueban el transporte
real del hook.

La integración y los artefactos actuales están en [P5-R2](p5-integration.md).
Las referencias siguientes al P5 anterior describen el checkpoint inicial;
sus archivos históricos se conservan separados de la entrega regenerada.

Fecha: 2026-11-14. Rama `develop`, base
`46e3ee05fc29ad090a1d6f7b1cb5e3dca9855350`, cambios locales sin commit ni despliegue.
T36 queda conciliado y probado de forma determinista, incluyendo integración
aislada con el transporte real de SourceMod y una VM SourcePawn. No se ejercita
una autenticación Steam real ni se instala nada en SRCDS.

## Diagnóstico y contrato conservado

`SteamWorks_BeginAuthSession` es un forward interno registrado, sin declaración
en el include público y sin consumidores de producto encontrados en el corpus
P0. Se conserva su nombre, `ET_Ignore`, tres argumentos y registro
`Param_Array, Param_Cell, Param_Cell`. No se añade una declaración pública.

La implementación anterior enviaba el primer parámetro con `PushStringEx`.
`CForward::PushStringEx` comprueba el tipo registrado y devuelve `SP_ERROR_PARAM`
para `Param_Array`. Por eso no bastaba con marcar el contenido como binario:
el transporte rechazaba el argumento antes de invocar al consumidor.

Ahora se envía con `PushArray`, conservando la representación de bytes compactos
en celdas Pawn de 32 bits. No se convierte cada byte en una celda. Se copia
exactamente la longitud indicada a almacenamiento alineado propio; la última
celda se rellena con ceros para no leer fuera del ticket original.

| Argumento | Representación y alcance |
|---|---|
| 1 | Buffer binario compacto, consumible como `char[]`; no es texto ni garantiza NUL dentro de la longitud válida |
| 2 | Longitud original en bytes, sin contar padding ni terminador |
| 3 | AccountID de 32 bits, preservando todos los bits; no SteamID64 |

Con longitud cero se suministra una celda cero de respaldo y se informa cero
bytes válidos, también si el puntero original es nulo. Un tamaño negativo o
puntero nulo con tamaño positivo omite la notificación. El engine sigue siendo
responsable de la validez del rango no nulo recibido de su propia llamada.

No hay copyback. Cada receptor recibe una copia del snapshot; sus escrituras no
alteran el ticket del engine ni la copia de otro receptor. Si no hay consumidores,
no se lee ni copia el ticket. Si falla la reserva, se omite la notificación; si
falla un push, se cancela el despacho parcial. El hook permanece observador:
`MRES_IGNORED` conserva la llamada y el resultado de autenticación del original.
No se registran ni almacenan tickets reales en la evidencia de pruebas.

## Pruebas deterministas

`scripts/test-auth-forward.sh` genera en `.build` la declaración de `CForward`
y sus métodos sin modificarlos desde el checkout SourceMod seleccionado. Omite
solo includes y el manager global ajenos al caso. Usa sus interfaces públicas y
`ReentrantList` reales, un adaptador de creación de forwards y SourceHook simulado.
No modifica el checkout de SourceMod.

Dos instancias de un SMX sintético ejecutan el callback en una VM SourcePawn
real. El inspector compara bytes y longitudes sin imprimir contenido. El receptor
modifica su primer byte, lo que comprueba aislamiento y ausencia de copyback.
Los hashes de entradas SourceMod, compilador, VM y fixture se registran.

- 227 comprobaciones por arquitectura, host x86_64 y Linux x86, con UBSan en el
  harness y métodos CForward compilados. La VM precompilada no está instrumentada.
- Cero, uno y dos consumidores; longitudes 0, 1, 3, 4, 5, 7, 8 y 1024; ceros
  intermedios, bytes altos, origen desalineado y reserva de tamaño exacto.
- Padding definido, AccountID con bit alto, puntero nulo, tamaño negativo y
  propietario detenido; liberación de forwards y runtimes de prueba.
- Control negativo con `CForward` real: el envío anterior devuelve
  `SP_ERROR_PARAM` y no llama al receptor. Una mutación local que restaura
  únicamente el envío anterior hace fallar la regresión; no altera producción.
- Ocho comprobaciones nuevas en la suite habitual de hooks (50 totales), incluidas
  fallas inducidas en los tres pushes, cancelación y ausencia de ejecución parcial.
  Se ejecutan también mediante el paso de lifecycle existente en CI Linux.
- Nueve suites integradas pasan en ambas arquitecturas con
  `test-integration.py --with-auth-forward`. El inventario público completo,
  incluido el registro del forward interno, sigue siendo idéntico a P0.
- Build completo Linux x86 con `-Werror` correcto; paquete local validado.

ASan/LSan pasa en las suites nativas de lifecycle, incluidas las nuevas guardas
de autenticación. La ejecución adicional con VM precompilada bajo ASan no se
acepta: al destruir su entorno aparece `new-delete-type-mismatch` dentro de
`sp::Environment::~Environment`, con una reserva de 264 bytes liberada como base
de 8 bytes. No se desactiva esa comprobación ni se atribuye a producción una
certificación ASan de la VM. El harness libera el entorno mediante `delete`,
según su API. Los diagnósticos iniciales y finales permanecen en `.build/t36/`.

La investigación posterior [SourcePawn y ASan](sourcepawn-vm-asan.md) identifica
y corrige la propiedad de motores y el cierre del entorno en una VM de pruebas
compilada desde fuentes fijadas. Las 227 comprobaciones pasan con esa VM bajo
ASan/LSan en ambas arquitecturas. El precompilado y este resultado histórico no
se sobrescriben ni se presenta la corrección como despliegue.

## Reproducción y evidencia

```bash
SOURCEMOD_DIR=/ruta/sourcemod \
SPCOMP=/ruta/sourcemod-runtime/scripting/spcomp64 \
SOURCEPAWN_VM=/ruta/sourcemod-runtime/bin/x64/sourcepawn.vm.so \
TEST_BUILD_DIR="$PWD/.build/auth-host" bash scripts/test-auth-forward.sh
```

Para x86, usar `TEST_CXXFLAGS=-m32` y una VM de 32 bits. La compilación del SMX
puede usar `spcomp64` en un host Linux de 64 bits. El script requiere ambas rutas
explícitas y falla si faltan; no descarga ni sustituye dependencias. Para la
regresión conjunta, añadir `--with-auth-forward` al ejecutor de integración y
proveer también `AMTL_DIR`. Sin esa opción, el ejecutor mantiene sus ocho suites
previas y la suite de hooks incluye las guardas nuevas.

Resultados, hashes y límites: [validation-results.json](t36/validation-results.json).
El nuevo build está en `.build/t36/build/package/`. Los ZIP y hashes de P5 son
capturas anteriores a T36, permanecen intactos y no contienen esta corrección.
No debe atribuirse a este binario la evidencia de ejecución SRCDS del binario P4.

## Riesgos y reversión

La copia temporal añade una reserva proporcional al ticket solo cuando existe
un consumidor. No se cambia la decisión de autenticación ni se interpreta su
contenido. La prueba no demuestra el ABI real de SourceHook en un evento Steam
ni la ejecución de un consumidor externo no inventariado.

La intervención futura debe usar un conjunto binario/include/gamedata coherente
y restaurarlo con el proceso detenido, según [la reversión revisada](p5-rollback.md).
No se recomienda reinstalar el envío defectuoso como recuperación operativa.
Para comparar fuentes, la corrección está limitada a `BeginAuthSession` y sus
includes C++ de apoyo; no revertir el archivo entero contra HEAD porque contiene
los cambios anteriores de P3 que aún no tienen commit.
