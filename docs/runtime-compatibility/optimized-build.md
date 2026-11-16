# Build x86 optimizado: límites de tickets y respaldo de detours

La compilación completa con `--enable-optimize` (`-O3 -DNDEBUG -m32`) y el build
canónico pasan con `-Werror`. Se conservan la API pública, el registro del forward,
el ticket binario empaquetado, su longitud en bytes y la decisión del motor de
Steam. No se añadieron supresiones de warnings ni de sanitizadores. Trabajo en
`develop`, sin commit, publicación, instalación ni operación de SRCDS.

## Causa y corrección

`BeginAuthSession` aceptaba un `int` positivo y redondeaba sus bytes a celdas de
4 bytes antes de reservar una matriz inicializada a cero. En x86, `INT_MAX-2`,
`INT_MAX-1` e `INT_MAX` se redondean a 2147483648 bytes, por encima del tamaño máximo
de objeto `PTRDIFF_MAX` (2147483647). GCC 12 rechaza ese `memset` implícito con
`-Werror=stringop-overflow` al optimizar.

`AuthTicketCellCount` calcula el techo con cociente/resto y comprueba el tamaño
acolchado contra `PTRDIFF_MAX` antes de reservar o leer. Conserva una celda para
la longitud cero. El máximo representable en x86 es 2147483644 bytes; las tres
longitudes superiores se omiten con `begin_auth_skip reason=ticket_too_large`.
El hook devuelve `MRES_IGNORED`, como ante una entrada inválida o un fallo de
reserva: solo se omite el snapshot para el forward, sin decidir por Steam si el
ticket es válido. Las longitudes representables siguen usando `new (nothrow)`;
la validación del tamaño no garantiza que exista memoria disponible.

El build completo reveló otro diagnóstico de la misma clase en CDetour: una
longitud del decodificador se copiaba a `patch_t::patch[20]` sin límite. Se valida
el resultado positivo y su capacidad antes de copiar o construir el trampoline.
Si no cabe, la creación falla con diagnóstico y sin modificar el destino del
hook. Esta comprobación mínima permite completar el build optimizado sin ocultar
el segundo error.

## Evidencia determinista

- Control negativo: el archivo original completo de `swgshooks.cpp`, compilado con
  los flags optimizados reales, vuelve a fallar por la reserva de 2147483648 bytes.
  El original de CDetour también reproduce el desbordamiento de su respaldo.
- 88 comprobaciones de hooks/lifecycle, incluyendo cálculo de longitudes 0–9,
  negativos, límites superiores sin reservar GiB y rechazo de los tres tamaños
  inválidos con una fuente diminuta. No se ejecuta ni se empuja el forward;
  diagnóstico escalar y resultado/meta originales conservados.
- 318 comprobaciones del respaldo real de CDetour: longitudes 1–20, contenido,
  cola intacta, rechazo de 0/21/22/SIZE_MAX antes de leer una fuente nula y sin
  modificar el respaldo previo. También pasan los 27 escenarios del owner/wrapper.
- El runner habitual compila estos casos con `-O3 -DNDEBUG` y checks explícitos
  que permanecen activos. La matriz integrada de 14 suites x86 pasa y compara
  declaraciones/registros/tipos/opcionalidad con P0. Tras ampliar el caso negativo
  con INT32_MIN se repitió la suite de lifecycle, con y sin ASan.
- ASan/LSan/UBSan sin supresiones: lifecycle, streaming, convivencia y respaldo de
  CDetour; 227 comprobaciones del forward con CForward/VM y 16 ciclos de propiedad
  de la VM. Solo datos sintéticos y hosts simulados; sin autenticación real nueva.

[Resultados y hashes](optimized-build/results.json),
[matriz integrada](optimized-build/integration.json),
[última suite de hooks y sanitizadores](optimized-build/hooks-final.log),
[CDetour instrumentado](optimized-build/asan-detours.log),
[control negativo del hook](optimized-build/negative-control.json) y
[control negativo de CDetour](optimized-build/negative-cdetour.json).

## Alcance posterior a P5-R8

Los ZIP R8 se conservan intactos y describen el checkpoint anterior. Esta corrección
cambia los binarios: las aceptaciones SRCDS de `82347b99…` siguen atribuidas a ese
SHA. El [seguimiento SRCDS del optimizado](optimized-srcds-acceptance.md) acepta
directamente `839ff075…` para dos reinits, HTTP/streaming, autenticación humana,
identidad/stats y quit limpio. El build canónico `75a9b5d9…` sigue sin ensayo
SRCDS. El seguimiento [P5-R15](p5-integration.md) renueva integración, paquetes y reversión
con ese ELF optimizado. La VM diagnóstica no se instala; las atribuciones se
conservan por hash. El [seguimiento T26/T35](p5-r9-rollback-srcds-acceptance.md)
acepta además instalación desde los ZIP R9 reales y reversión en sitio con
arranque/Steam/HTTP del baseline; su fallo de salida sigue preservado.
