# Instalación y reversión

## Requisitos y archivos

Usa SourceMod y Metamod:Source compatibles con L4D2 y un runtime Steam admitido por
el [selector nativo](../runtime/interfaces.md). Para Linux instala un ELF32 x86;
una extensión x64 no sirve al motor x86. Windows tiene un build independiente.

Del paquete copia únicamente `addons/sourcemod/extensions/steamworks.ext.so`
(o `.dll` en Windows) y `addons/sourcemod/scripting/include/steamworks.inc`.
El include sirve para compilar plugins y debe corresponder al binario instalado.
La distribución actual no instala gamedata de SteamWorks: los perfiles JSON de
pruebas no son configuración. Conserva README, licencia y documentación como referencia.
No copies SDK, bibliotecas Steam de compilación, fixtures, SMX de pruebas o VM instrumentada.

## Actualización

1. Identifica la plataforma y registra hashes de extensión, include y cualquier
   `gamedata/steamworks.txt` anterior; registra también su ausencia.
2. Con el proceso detenido, captura esos archivos propios fuera del árbol que
   vas a reemplazar. Conserva permisos, configuraciones y plugins ajenos.
3. Extrae el paquete en un directorio temporal, comprueba contenido y hashes,
   y copia los archivos propios a su destino.
4. Arranca mediante el administrador habitual y comprueba carga de extensión,
   conexión Steam y capacidades necesarias. El éxito de carga no valida todos
   los consumidores ni una respuesta remota GC.

## Reversión

Detén el proceso y restaura exactamente los archivos capturados. Si un archivo
propio estaba ausente en el baseline, retira solo ese archivo; no inventes
un gamedata ni borres configuraciones ajenas. Reinicia y verifica hashes,
carga y funcionalidades usadas. No sobrescribas todo SourceMod para revertir
una extensión. La descarga/recarga soportada y el cambio en disco son ensayos distintos.

La herramienta de fuentes `scripts/package/prepare-review-artifacts.py` prepara
candidato y rollback desde una captura local previa, exige salida nueva y prueba
el roundtrip en un directorio desechable. No accede a servidores ni acredita por
sí sola la reversión en sitio. Consulta [compilación](../build/README.md).
