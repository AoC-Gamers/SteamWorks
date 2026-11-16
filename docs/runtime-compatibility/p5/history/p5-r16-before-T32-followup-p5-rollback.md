# Reversión de P5-R16

La captura R16 se renovó por lectura de los archivos propios de `competitive_dev`,
sin iniciar/detener procesos ni modificar el servidor. Coincide con los bytes de
baseline R7/R8/R9. El seguimiento [R9 en sitio](p5-r9-rollback-srcds-acceptance.md)
aplicó el ZIP con el mismo SHA después del mismo candidato optimizado y verificó
arranque/Steam/HTTP. R16 conserva los bytes del ZIP rollback y reproduce el
payload extensión/include instalado para T11 en R14; no modifica el comentario
ni el comportamiento de RestartRequested.
Se renuevan documentación/captura y verificaciones locales, sin otro ensayo SRCDS.
Los paquetes se conservan intactos; el baseline mantiene su fallo de salida.

## Conjunto guardado

`.build/p5-r16/review/steamworks-p5-r16-competitive-dev-rollback.zip` conserva:

- `extensions/steamworks.ext.so`: SHA-256
  `9ca37aadcd83448d49126d6a72babcb8ec97ec5a06c171b4821ae0ee3cf05d70`;
- `scripting/include/steamworks.inc`: SHA-256
  `e51bcd397139edc3d144f130c45f0f8edbf55d12278736e0134e4c650ec64f17`;
- ausencia de `gamedata/steamworks.txt`, declarada en el manifiesto. Revertir exige
  retirarlo si una intervención posterior lo hubiera creado.

[Captura R16](p5/r16/baseline-observation.json). El ZIP de rollback conserva SHA-256
`55153d557f3af7f71f1016c8c394dc74fe29e0cdc9511044c21de29e8bc6dffc`, idéntico al
R9 probado. No incluye configuraciones privadas, SQLite, credenciales, SDK ni
módulos del juego. El runtime candidato solo contiene extensión/include; la VM
diagnóstica permanece separada y no se instala/restaura con este procedimiento.

## Evidencia y límites

| Comprobación | Alcance |
|---|---|
| Roundtrip R16 candidato → ZIP rollback | Bytes reales en filesystem temporal; hashes restaurados, gamedata ausente retirado y config ajena preservada |
| Paquete R16 | Árbol y ZIP extraído validados; solo ELF32 optimizado/include en runtime |
| Preparador | Doce pruebas de arquitectura, symlinks, contenido, ausencia, repetibilidad y no sobrescritura |
| [Reversión en sitio R7](p5-r7-rollback-srcds-acceptance.md) | Histórica, después de `82347b99…`: ZIP aplicado al mismo árbol detenido, hashes/ausencia/configs preservados; nuevo PID, Steam y HTTP pasan; baseline con SIGSEGV final |
| [Restauración tras aceptación optimizada](optimized-srcds/cleanup.json) | Selector, cuatro hashes y módulo original verificados; DEV conectado y laboratorio retirado. No aplicó el ZIP de rollback |
| [Reversión en sitio R9 después de `839ff075…`](p5-r9-rollback-srcds-acceptance.md) | ZIP real aplicado al mismo árbol detenido; hashes/ausencia/configs preservados, nuevo PID y Steam/HTTP correctos; SIGSEGV final del baseline conservado |

La [aceptación optimizada](optimized-srcds-acceptance.md) acredita quit limpio de
`839ff075…` después de dos reinicializaciones con SourceTVSupport y pruebas humanas.
**El baseline archivado conserva su fallo de salida.** Restaurarlo recupera los
bytes originales sin incorporar las correcciones nuevas. Restauración/arranque y
terminación limpia son comprobaciones distintas; el cierre del baseline no se acepta.

T26/T35 conservan la aceptación del ensayo R9 por los bytes comprobados. El nuevo
ZIP candidato R16 renueva documentación/metadatos; extensión/include idénticos a R15 instalado para T25, sin instalar el ZIP R16; el
archivo de rollback R16 sí es idéntico, incluido su SHA, al ejercitado en R9.

El [seguimiento T20](t20-owner-srcds-acceptance.md) también restauró selector,
cuatro hashes y DEV conectado tras descargar/recargar solo el plugin de pruebas.
No aplicó un rollback ni descargó la extensión; T35 sigue acreditado por R9.

El [seguimiento T16](t16-extension-srcds-acceptance.md) restauró el DEV tras carga
tardía y dos ciclos de unload/recarga serial con pila mínima. Solo se descargó
la extensión sin consumidores/peticiones activas; no se aplicó rollback de archivos.
T35 sigue acreditado por el ZIP real de R9, idéntico al regenerado en R16.

El [seguimiento T15/T29 de R12](identity-negatives-srcds-acceptance.md) también
restauró el selector y los cuatro hashes protegidos, reconectó DEV y eliminó el
laboratorio tras quit controlado. Esa restauración de laboratorio no aplicó el
ZIP rollback: T35 mantiene la atribución al ensayo R9. R16 recompila el harness
y renueva los paquetes localmente; no opera el gameserver.

El [seguimiento T23 de R13](forward-consumers-srcds-acceptance.md) también restauró
selector, cuatro hashes y DEV conectado tras su matriz de forwards y quit limpio.
No aplicó el ZIP de rollback. R16 conserva sus tests y comentario corregido,
repite el roundtrip en filesystem temporal y conserva el ZIP de reversión
idéntico a R9; no opera el gameserver ni vuelve a aceptar el cierre del baseline.

El [seguimiento T11 de R14](interface-capabilities-srcds-acceptance.md) restauró
selector, cuatro hashes, módulo original y DEV conectado tras clasificar las
interfaces y probar GC local/rechazo/restauración. Solo el gamedata propio del
laboratorio cambió; fue retirado. No aplicó el ZIP rollback en sitio ni acredita
GC remoto. R16 incorpora esas fuentes/evidencias y captura por lectura el mismo
baseline, sin operar el gameserver.

El [seguimiento T25 de R15](consumer-compat-srcds-acceptance.md) restauró DEV y
retiró el laboratorio tras seis variantes y H08/H09 humanos. T25 sigue parcial.
Esa restauración no aplicó el ZIP de rollback. R16 incorpora la evidencia y
fuentes, repite validación y roundtrip locales, sin tocar el proceso ni renovar
la aceptación en sitio de T35. El ZIP rollback conserva el SHA ejercitado en R9.

## Procedimiento para una intervención autorizada

1. Identificar destino/supervisor, verificar manifiesto y `SHA256SUMS`, y contrastar
   la captura con el estado real. Si cambió, capturar primero el conjunto actual.
2. Detener completamente SRCDS mediante LinuxGSM y comprobar la salida. No sustituir
   la restauración con proceso detenido por un unload en caliente.
3. Restaurar binario/include y retirar `gamedata/steamworks.txt` si fue creado.
   Restaurar solo las configuraciones efectivamente cambiadas; conservar CFG,
   datos ajenos y bibliotecas del juego.
4. Comprobar hashes/ausencias. Si hubo un selector aislado, restaurar el guardado,
   manteniendo flags, contraseña, SourceTV y CFG habituales.
5. Arrancar mediante el supervisor original y comprobar módulo mapeado, consumidores,
   logs, Steam y operaciones afectadas. Ante fallo, detener y conservar evidencia.
6. Retirar solo temporales propios tras verificar restauración. No distribuir
   configuraciones privadas, tickets, comandos con credenciales ni cores.

Los paquetes y la [documentación anterior](p5/history/p5-r15-with-T25-partial-followup-p5-rollback.md)
se conservan intactos. Ni el ZIP ni su validación autorizan publicación o despliegue;
todo sigue en `develop`, sin commit.
