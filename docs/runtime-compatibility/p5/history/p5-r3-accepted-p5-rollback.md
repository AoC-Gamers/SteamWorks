# Reversión de P5-R3

El conjunto actual se prepara localmente en `develop`, sin commit ni despliegue.
La captura se renovó por lectura de `competitive_dev` y coincide con el baseline
anterior. No se arrancó, detuvo ni modificó ningún servidor en esta actualización.

## Conjunto guardado

`.build/p5-r3/review/steamworks-p5-r3-competitive-dev-rollback.zip` conserva:

- `extensions/steamworks.ext.so`: SHA-256
  `9ca37aadcd83448d49126d6a72babcb8ec97ec5a06c171b4821ae0ee3cf05d70`;
- `scripting/include/steamworks.inc`: SHA-256
  `e51bcd397139edc3d144f130c45f0f8edbf55d12278736e0134e4c650ec64f17`;
- ausencia de `gamedata/steamworks.txt`, registrada en `manifest.json` para retirar
  un archivo nuevo al revertir, si lo hubiera creado una intervención.

La [captura con fecha y hashes](p5/r3/baseline-observation.json) y el manifiesto
identifican archivos reales, sin configs privadas, datos SQLite, credenciales,
SDK ni módulos del motor/Steam. El candidato tiene solo extensión e include como
payload runtime. Las VMs instrumentadas son diagnóstico separado y nunca se
instalaron en el servidor; no pertenecen a su reversión.

## Qué se validó

| Validación | Resultado y alcance |
|---|---|
| Nuevo candidato → ZIP de reversión | Pasa en filesystem temporal con bytes reales; restaura hashes, elimina gamedata ausente y conserva una config ajena |
| Preparador de artefactos | Doce pruebas pasan; arquitectura, symlinks, contenido, ausencia, repetibilidad y no sobrescritura |
| Restauración del laboratorio 014 | Ejecutada: selector/config originales y extensión anterior mapeada, DEV conectado a Steam |
| Restauración del laboratorio streaming | Ejecutada tras `quit`: mismos hashes originales, DEV conectado, árbol aislado y fixture retirados |
| Restauración en sitio desde el nuevo ZIP con arranque | Pendiente; no se simula como aprobada por las filas anteriores |

Las restauraciones reales están registradas en
[014](auth014-srcds/cleanup.json) y [streaming](http-stream-srcds/cleanup.json).
Esos ensayos cambiaron el selector hacia una copia aislada y lo restauraron;
no instalaron el ZIP de reversión sobre los archivos originales. La distinción
se conserva en T35 y en la [matriz P5](p5/validation-results.json).

La [aceptación de autenticación con el candidato final](p5-r3-auth-srcds-acceptance.md)
añade otra restauración real del selector aislado, con los mismos hashes y
conexión a Steam. Su payload provino del ZIP de candidato; el ZIP de reversión
sigue sin instalarse en sitio. Los ZIP anteriores no se regeneraron.

## Procedimiento para una futura intervención autorizada

1. Identificar destino y supervisor; mantener acceso administrativo independiente
   de SteamWorks. Comprobar checksums del ZIP y contrastar la captura con el estado
   actual. Si cambió el destino, capturar su conjunto antes de intervenir.
2. Detener completamente SRCDS con su supervisor y comprobar la salida. No usar
   unload/reload en caliente como recuperación de archivos incompatibles.
3. Restaurar binario/include/gamedata y las configuraciones que realmente se hayan
   cambiado. Para este baseline, retirar `gamedata/steamworks.txt` si la intervención
   lo creó; no eliminar configs/datos ajenos ni sustituir bibliotecas del juego.
4. Comprobar hashes y ausencias. Si se usó un `sm_basepath` aislado, restaurar el
   selector guardado, sin inventar parámetros/contraseña/SourceTV ni CFG de juego.
5. Arrancar mediante el supervisor original y comprobar módulos cargados,
   consumidores, logs, conexión a Steam y operaciones afectadas. Ante fallo,
   detener y conservar evidencia antes de otra acción.
6. Retirar solo los archivos temporales propios después de verificar la
   restauración. Conservar la evidencia filtrada sin configuraciones privadas.

El ZIP es una captura verificable, no una copia universal ni una firma de
publicación. Los checkpoints y paquetes P5/P5-R2 previos permanecen intactos;
la [preparación anterior](p5/history/p5-r2-rollback-original.md) se conserva como
historia. El nuevo manifiesto no autoriza publicación ni despliegue.
