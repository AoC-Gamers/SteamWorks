# Reversión de P5-R10

La captura R10 se renovó por lectura de los archivos propios de `competitive_dev`,
sin iniciar/detener procesos ni modificar el servidor. Coincide con los bytes de
baseline R7/R8/R9. El seguimiento [R9 en sitio](p5-r9-rollback-srcds-acceptance.md)
aplicó el ZIP con el mismo SHA después del mismo candidato optimizado y verificó
arranque/Steam/HTTP. R10 conserva los bytes del payload candidato y del ZIP rollback;
solo renueva documentación/captura y verificaciones locales, sin otro ensayo SRCDS.
Los paquetes se conservan intactos; el baseline mantiene su fallo de salida.

## Conjunto guardado

`.build/p5-r10/review/steamworks-p5-r10-competitive-dev-rollback.zip` conserva:

- `extensions/steamworks.ext.so`: SHA-256
  `9ca37aadcd83448d49126d6a72babcb8ec97ec5a06c171b4821ae0ee3cf05d70`;
- `scripting/include/steamworks.inc`: SHA-256
  `e51bcd397139edc3d144f130c45f0f8edbf55d12278736e0134e4c650ec64f17`;
- ausencia de `gamedata/steamworks.txt`, declarada en el manifiesto. Revertir exige
  retirarlo si una intervención posterior lo hubiera creado.

[Captura R10](p5/r10/baseline-observation.json). El ZIP de rollback conserva SHA-256
`55153d557f3af7f71f1016c8c394dc74fe29e0cdc9511044c21de29e8bc6dffc`, idéntico al
R9 probado. No incluye configuraciones privadas, SQLite, credenciales, SDK ni
módulos del juego. El runtime candidato solo contiene extensión/include; la VM
diagnóstica permanece separada y no se instala/restaura con este procedimiento.

## Evidencia y límites

| Comprobación | Alcance |
|---|---|
| Roundtrip R10 candidato → ZIP rollback | Bytes reales en filesystem temporal; hashes restaurados, gamedata ausente retirado y config ajena preservada |
| Paquete R10 | Árbol y ZIP extraído validados; solo ELF32 optimizado/include en runtime |
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
ZIP candidato R10 cambia documentación/metadatos y no se vuelve a instalar; el
archivo de rollback R10 sí es idéntico, incluido su SHA, al ejercitado en R9.

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

Los paquetes y la [documentación anterior](p5/history/p5-r9-with-install-rollback-followup-p5-rollback.md)
se conservan intactos. Ni el ZIP ni su validación autorizan publicación o despliegue;
todo sigue en `develop`, sin commit.
