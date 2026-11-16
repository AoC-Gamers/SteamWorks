# Preparación y reversión de P5

Este procedimiento conserva la preparación inicial. La ejecución posterior en el
laboratorio LinuxGSM y la restauración de DEV se registran por separado en
[aceptación SRCDS](srcds-acceptance.md).

Este procedimiento está preparado para una futura intervención autorizada.
P5 no instala archivos ni arranca o detiene servidores. La prueba realizada es
una restauración de archivos en un directorio temporal, no T35-I en SRCDS.

## Conjunto guardado

La captura de solo lectura de `competitive_dev` conserva:

- `extensions/steamworks.ext.so`, SHA-256
  `9ca37aadcd83448d49126d6a72babcb8ec97ec5a06c171b4821ae0ee3cf05d70`;
- `scripting/include/steamworks.inc`, hash en el manifiesto;
- ausencia de `gamedata/steamworks.txt`, registrada explícitamente.

La extensión no crea un archivo de configuración propio. No se incluyen configs
de servidor/plugins, credenciales, SDK ni módulos Steam. El candidato tampoco
cambia esas configuraciones. Si una intervención futura modifica configuración
adicional, deberá guardarla y ampliar este conjunto antes de empezar.

Los ZIP y `SHA256SUMS` están en `.build/p5-r2/review/`; `manifest.json` registra
contenido, hashes y archivos anteriormente ausentes. El ZIP de reversión se
refiere exclusivamente a esta captura. No sustituye una copia del estado del
destino inmediatamente antes de intervenir. Sus hashes permiten detectar cambios
accidentales; no son una firma de publicación.

## Procedimiento revisado

1. Identificar el servidor, comprobar el proceso y mantener acceso administrativo
   independiente de SteamWorks. Verificar los checksums del conjunto guardado.
2. Detener por completo SRCDS mediante el supervisor de ese destino. Comprobar
   que salió; no forzar unload/reload como método de recuperación.
3. Restaurar juntos el binario, include, gamedata y cualquier configuración que
   realmente se hubiera cambiado. Para esta captura, retirar `steamworks.txt`
   si la intervención hubiera creado uno: su estado anterior era ausente.
4. Comprobar hashes restaurados y ausencia de archivos indicados en el manifiesto.
   No reemplazar librerías Steam, engine ni SourceMod para ajustar el resultado.
5. Arrancar mediante el supervisor original. Revisar carga de extensión y
   consumidores, logs, conexión y operaciones utilizadas; comparar con el
   baseline. Ante fallo, detener y conservar evidencia antes de otro intento.

`scripts/prepare-review-artifacts.py` verifica los ZIP y restaura el conjunto
archivado en un sandbox con bytes reales de la captura. Comprueba también que
un archivo de gamedata nuevo se retire y que una configuración ajena permanezca
intacta. Once pruebas adicionales cubren arquitectura errónea, archivos faltantes,
symlinks, payload inesperado, gamedata existente, repetibilidad y no sobrescritura.
Esto valida el contrato de archivos; el arranque del binario anterior tras una
reversión real queda pendiente para una intervención autorizada.

## Actualización P5-R2

La captura se renueva por lectura de `competitive_dev` y conserva los mismos
hashes del binario y del include anteriores. El preparador etiqueta los nuevos
ZIP como `p5-r2`, sin sobrescribir el conjunto inicial. La prueba de filesystem
instala los bytes reales del candidato actual en el sandbox y restaura los del
ZIP anterior; comprueba la ausencia de gamedata y conserva una configuración ajena.

La VM instrumentada y los parches SourcePawn pertenecen al diagnóstico aislado.
El candidato no instala esa VM; por tanto, la reversión del servidor no incorpora
bibliotecas SourcePawn de pruebas. Para revertir el diagnóstico local basta con
dejar de seleccionar su directorio. Antes de una futura intervención se debe
comprobar nuevamente que la captura coincide con el destino.
