# P5-R9: aceptación de instalación y reversión en sitio en SRCDS x86

2026-11-15. **T26 y T35 pasan en su alcance ejecutado:** instalación del ZIP real,
reversión en el mismo árbol detenido y arranque/Steam/HTTP del baseline restaurado.
El candidato optimizado sale limpiamente. El baseline archivado reproduce su
SIGSEGV conocido al salir; su cierre limpio no se acepta. P5 sigue sin aceptación
global. `develop`, API pública intacta, sin commit ni producción.

## Archivos realmente utilizados

| Elemento | SHA-256 |
|---|---|
| ZIP candidato R9 | `327fc386c05454c90b8070f000102f8afbf143ff02e5092228942a8279e1dba1` |
| Extensión optimizada instalada | `839ff0751b04141031bdc6dffc48662d0804f52227f2c91015bd87c6b84cf192` |
| ZIP rollback R9 | `55153d557f3af7f71f1016c8c394dc74fe29e0cdc9511044c21de29e8bc6dffc` |
| Extensión restaurada | `9ca37aadcd83448d49126d6a72babcb8ec97ec5a06c171b4821ae0ee3cf05d70` |
| Include restaurado | `e51bcd397139edc3d144f130c45f0f8edbf55d12278736e0134e4c650ec64f17` |

Los ZIP se verificaron contra manifiesto/checksums y se conservaron intactos.
Se usó `competitive_dev`, LinuxGSM `/app/l4d2server`, SRCDS x86 y 27015, con copia
aislada en `addons/sourcemod_acceptance`, flags/configuraciones habituales y sin
añadir contraseña. La VM y las bibliotecas Steam/engine no se sustituyeron.

## Secuencia y comprobaciones

1. Con SRCDS detenido se abrió el ZIP candidato dentro del contenedor. Se
   verificaron sus 663 miembros y se instalaron únicamente extensión/include del
   payload runtime, comprobando hashes y ausencia de gamedata. El baseline original
   se contrastó antes con el ZIP de reversión. No se instaló un binario suelto del build.
2. LinuxGSM arrancó el candidato, PID **92999**. Dispositivo/inodo/hash confirman
   el optimizado realmente mapeado. Steam conectado, 135 plugins sin filas de
   error y HTTP 200: cuerpo de 16 bytes, callback y ambas celdas del contexto
   correctos. El handle cerró de forma diferida; no hubo logs de errores SourceMod.
3. El candidato completó `quit` sin marcadores de crash ni nuevo core asociado.
   Tras comprobar que no quedaba SRCDS, se abrió el ZIP rollback dentro del guest
   y se restauraron sus bytes en ese mismo árbol. No se simuló cambiando al selector
   del DEV original.
4. Binario/include recuperaron sus hashes originales y se retiró el gamedata
   declarado ausente. Para probar su retirada se creó un testigo sintético solo
   después de detener el candidato: **no lo generó el candidato ni se cargó**.
   Selector, `configs/core.cfg`, CFG testigo ajena y originales protegidos se conservaron.
5. LinuxGSM arrancó el baseline, PID nuevo **94840**, con sus bytes realmente
   mapeados. Steam y el mismo HTTP/cuerpo/contexto/callback pasaron. Las 135 filas
   de plugins fueron idénticas y no hubo errores SourceMod durante la operación.
6. Al pedir `quit`, el baseline mostró `Segmentation fault` y un evento systemd de
   señal 11 asociado a ese PID. Se conserva este fallo, consistente con el ya
   conocido; no se volvió a investigar su causa ni se suprimieron comprobaciones.
7. Se restauró byte a byte el selector original y LinuxGSM arrancó el DEV habitual,
   PID **96710**, conectado a Steam y con la extensión original mapeada. Se
   comprobaron los cuatro hashes protegidos y se retiraron laboratorio/recovery y fixture.

El mismo harness compila sin warnings contra ambos includes y produce SMX idéntico.
Su cierre por timer es el protocolo común compatible con el baseline antiguo.
La fixture recibió exactamente dos peticiones `/text`. No se necesitó jugador ni
se repitieron autenticación, identidad, stats o GC. Su aceptación previa en el
mismo ELF optimizado conserva el [informe independiente](optimized-srcds-acceptance.md).

## Incidencia y límites

La [auditoría de cores](p5-r9-rollback-srcds/core-audit.json) identifica el evento
del baseline y ninguno del candidato. Solo exporta metadatos mínimos, sin cores,
argumentos ni entorno; los cores originales de systemd se conservan. Las líneas
lifecycle del JSON de cierre del baseline pertenecen al log acumulado del candidato:
no se atribuyen como instrumentación del baseline.

T26 acredita el payload instalado desde el ZIP real; T35 acredita restauración,
coherencia de archivos y arranque/operación HTTP mínima después del optimizado.
No acreditan cierre limpio del baseline ni la matriz global. El candidato mantiene
su aceptación de lifecycle con SourceTVSupport y suma otro quit limpio.
Los pendientes restantes de interfaces/GC/consumidores, identidad y lifecycle
siguen en la [matriz activa](p5/validation-results.json).

## Evidencia conservada

[Resultados/hashes](p5-r9-rollback-srcds/results.json),
[instalación desde ZIP](p5-r9-rollback-srcds/candidate-installation.json),
[candidato](p5-r9-rollback-srcds/candidate-runtime.json),
[aplicación real del rollback](p5-r9-rollback-srcds/applied-rollback.json),
[baseline arrancado](p5-r9-rollback-srcds/rollback-runtime.json),
[módulos restaurados](p5-r9-rollback-srcds/rollback-modules.json),
[cierre candidato](p5-r9-rollback-srcds/candidate-shutdown.json),
[cierre baseline](p5-r9-rollback-srcds/rollback-shutdown.json) y
[restauración final](p5-r9-rollback-srcds/cleanup.json).

Paquete separado de revisión:
`.build/p5-r9-rollback-srcds/review/steamworks-p5-r9-install-in-place-rollback-srcds-acceptance.zip`,
con manifiesto y checksums. Incluye evidencia filtrada, harness e instalador/aplicador;
excluye configuraciones privadas, logs crudos, claves, SDK, módulos del juego y cores.
Los ZIP P5-R9 conservan su checkpoint previo al seguimiento. La
[matriz anterior](p5/history/p5-r9-before-install-rollback-acceptance-validation-results.json)
queda guardada; los paquetes existentes no se regeneraron.
