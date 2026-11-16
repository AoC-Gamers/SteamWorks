# Reversión de P5-R6

La captura R6 se renovó por lectura de los archivos propios en `competitive_dev`,
sin iniciar/detener procesos ni cambiar el destino. Coincide con el baseline R5.
El paquete nuevo es de revisión local; no se aplicó en sitio en esta tarea.

## Conjunto guardado

`.build/p5-r6/review/steamworks-p5-r6-competitive-dev-rollback.zip` conserva:

- `extensions/steamworks.ext.so`: SHA-256
  `9ca37aadcd83448d49126d6a72babcb8ec97ec5a06c171b4821ae0ee3cf05d70`;
- `scripting/include/steamworks.inc`: SHA-256
  `e51bcd397139edc3d144f130c45f0f8edbf55d12278736e0134e4c650ec64f17`;
- ausencia de `gamedata/steamworks.txt`, declarada en el manifiesto. Revertir exige
  retirarlo si una intervención posterior lo hubiera creado.

[Captura renovada](p5/r6/baseline-observation.json). No se guardan configuraciones
privadas, datos SQLite, credenciales, SDK ni módulos del juego. El candidato R6
solo añade extensión/include como runtime; la VM diagnóstica queda fuera de
`addons` y no se instala ni se restaura con este procedimiento.

## Evidencia y límites

| Comprobación | Alcance |
|---|---|
| Roundtrip R6 candidato → ZIP de reversión | Filesystem temporal, bytes reales: hashes restaurados, gamedata ausente retirado y config ajena preservada |
| Árbol candidato y extracción de ZIP R6 | Validador de paquete y checksums; solo ELF32/include en runtime |
| Preparador | Doce pruebas: arquitectura, symlinks, contenido, ausencia, repetibilidad y no sobrescritura |
| [Reversión en sitio R5](p5-r5-rollback-srcds-acceptance.md) | Histórica: baseline restaurado en copia aislada, arranque/Steam/HTTP correctos; exit SIGSEGV conservado |
| [Restauración tras T08](t08-coexist-srcds/cleanup.json) | Selector, cuatro hashes y módulo original verificados; DEV conectado, laboratorio retirado. No aplicó el ZIP R6 |
| [Restauración tras auth P5-R6](p5-r6-auth-srcds/cleanup.json) | Selector, cuatro hashes y módulo original verificados; DEV conectado, laboratorio retirado; no aplicó el ZIP de reversión |
| Reversión en sitio R6 | Pendiente; la preparación local y restaurar un selector no sustituyen ese ensayo |

La [aceptación T08](t08-coexist-srcds-acceptance.md) acredita cierre limpio del
candidato `82347b99…` tras dos reinicializaciones con SourceTVSupport.
**El baseline archivado conserva su fallo de salida.** Restaurarlo recupera
su conjunto original y no incorpora la corrección de T08. Arranque, restauración
de archivos y terminación limpia son comprobaciones diferentes.

## Procedimiento para una intervención autorizada

1. Identificar destino/supervisor, verificar manifiesto y `SHA256SUMS`, y contrastar
   la captura con el estado real. Si cambió, capturar primero su conjunto actual.
2. Detener completamente SRCDS con LinuxGSM y comprobar la salida; no usar unload
   en caliente como sustituto de restaurar archivos con el proceso detenido.
3. Restaurar binario/include y retirar `gamedata/steamworks.txt` si se había creado.
   Restaurar únicamente configuraciones efectivamente cambiadas; conservar CFG y
   datos ajenos y las bibliotecas del juego.
4. Comprobar hashes/ausencias. Si hubo un selector aislado, restaurar el guardado,
   sin inventar flags, contraseña, SourceTV ni CFG de juego.
5. Arrancar mediante el supervisor original y comprobar módulo mapeado, consumidores,
   logs, Steam y operaciones afectadas. Ante fallo, detener y conservar evidencia.
6. Retirar solo temporales propios después de verificar restauración. No distribuir
   configuraciones privadas, tickets, comandos con credenciales ni cores.

Los paquetes y [documentación anterior](p5/history/p5-r5-with-t08-followups-p5-rollback.md)
se conservan intactos. Ni el ZIP ni su verificación local autorizan publicación o
un despliegue; todo sigue en `develop`, sin commit.
