# Instalación y reversión

## Requisitos y archivos

Usa SourceMod y Metamod:Source compatibles con L4D2 y un runtime Steam admitido por
el [selector nativo](../runtime/interfaces.md). Para Linux instala un ELF32 x86;
una extensión x64 no sirve al motor x86. Windows tiene un build independiente.

Del paquete copia únicamente `addons/sourcemod/extensions/steamworks.ext.2.l4d2.so`
(o `.dll` en Windows) y `addons/sourcemod/scripting/include/steamworks.inc`.
El include sirve para compilar plugins y debe corresponder al binario instalado.
La identidad lógica solicitada por los plugins sigue siendo `steamworks.ext`;
SourceMod resuelve el sufijo de L4D2.
La distribución actual no instala gamedata de SteamWorks: los perfiles JSON de
pruebas no son configuración. Conserva README, licencia y documentación como referencia.
Los JSON de build y empaquetado no forman parte del ZIP instalable ni son
configuración de SourceMod. No copies SDK, bibliotecas Steam de compilación,
fixtures, SMX de pruebas o VM instrumentada.

## Actualización

1. Identifica la plataforma y registra hashes de extensión, include y cualquier
   `gamedata/steamworks.txt` anterior; registra también su ausencia. Si vienes del
   nombre genérico `steamworks.ext.so` (o `steamworks.ext.dll`), captura también
   ese archivo y la presencia o ausencia del nuevo nombre.
2. Con el proceso detenido, captura esos archivos propios fuera del árbol que
   vas a reemplazar. Conserva permisos, configuraciones y plugins ajenos.
3. Extrae el paquete en un directorio temporal, comprueba contenido y hashes,
   y copia los archivos propios a su destino. Retira de la instalación activa el
   binario genérico capturado: debe quedar solo el nombre específico de L4D2.
4. Arranca mediante el administrador habitual y comprueba carga de extensión,
   conexión Steam y capacidades necesarias. El éxito de carga no valida todos
   los consumidores ni una respuesta remota GC.

## Reversión

Elige un baseline identificado por hash y validado también al salir del proceso;
una carga correcta o un smoke HTTP no acreditan un cierre limpio. Si se prepara
un baseline corregido, conserva la captura anterior y documenta el nuevo origen,
SDK y opciones de build. No sustituyas sus bytes manteniendo la atribución antigua.

Detén el proceso y restaura exactamente los archivos capturados. Si un archivo
propio estaba ausente en el baseline, retira solo ese archivo. Al volver a un
baseline genérico, elimina el binario con sufijo L4D2 si antes estaba ausente y
restaura el nombre genérico original. SourceMod prioriza el nombre específico,
por lo que dejarlo impediría seleccionar el baseline. No inventes
un gamedata ni borres configuraciones ajenas. Reinicia y verifica hashes,
carga, funcionalidades usadas y salida controlada en una copia aislada. Conserva
los fallos y sus límites si el baseline no cierra limpiamente. La traza de
lifecycle puede confirmar el cierre de un contexto cliente activo con el
GameServer ya detenido; no fuerces un cierre sobre un GameServer activo para
obtener esa observación. No sobrescribas todo SourceMod para revertir
una extensión. La descarga/recarga soportada y el cambio en disco son ensayos distintos.

La herramienta de fuentes `scripts/package/prepare-review-artifacts.py` prepara
candidato y rollback desde una captura local previa, exige salida nueva y prueba
el roundtrip en un directorio desechable. No accede a servidores ni acredita por
sí sola la reversión en sitio. Consulta [compilación](../build/README.md).
