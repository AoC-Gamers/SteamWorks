# P5-R7: aceptación de reversión en sitio en SRCDS x86

2026-11-15. **T35 pasa para restauración, arranque y operación HTTP mínima.**
El candidato termina limpiamente. El baseline archivado reproduce su SIGSEGV
conocido al salir: su cierre limpio no se acepta. P5 conserva pendientes y no tiene
aceptación global. Rama `develop`, API pública intacta, sin commit ni producción.

## Conjunto probado

| Elemento | SHA-256 |
|---|---|
| ZIP candidato R7 | `9d944edeef897d11100bace180b8043d4f089f6b32c1a075286ba3592eda05eb` |
| Extensión candidata x86 | `82347b99d5130bd411cdc0b35d215dc87fc82d394a2252ccd83d5d4940b6ca5a` |
| ZIP rollback R7 | `55153d557f3af7f71f1016c8c394dc74fe29e0cdc9511044c21de29e8bc6dffc` |
| Extensión restaurada | `9ca37aadcd83448d49126d6a72babcb8ec97ec5a06c171b4821ae0ee3cf05d70` |
| Include restaurado | `e51bcd397139edc3d144f130c45f0f8edbf55d12278736e0134e4c650ec64f17` |

Los ZIP revisados se verificaron y conservaron intactos. Se usó `competitive_dev`,
LinuxGSM `/app/l4d2server`, SRCDS x86 y puerto 27015, con una copia SourceMod en
`addons/sourcemod_acceptance`. Se conservaron flags y configuraciones habituales,
sin añadir contraseña ni sustituir VM o bibliotecas Steam/engine.

## Secuencia observada

1. El candidato extraído del ZIP arrancó en PID **74831** y se verificó su módulo
   mapeado por hash/dispositivo/inodo. Steam conectado, 135 plugins y una petición
   HTTP loopback correcta: estado 200, 16 bytes esperados, callback y las dos celdas
   del contexto. No hubo logs de errores SourceMod. `quit` terminó sin marcadores
   de crash ni nuevo core asociado al candidato.
2. Con SRCDS detenido se abrió el ZIP de rollback dentro del contenedor y se
   restauraron sus bytes en el mismo árbol. No se simuló la reversión cambiando el
   selector al DEV original. Se verificaron hashes de extensión/include y retirada
   de `gamedata/steamworks.txt`. El testigo sintético para comprobar esa retirada
   se creó después de detener el candidato: **no lo generó el candidato ni se cargó**.
3. El selector permaneció igual durante la reversión; `configs/core.cfg` y una CFG
   testigo ajena al paquete conservaron sus hashes. Los originales protegidos
   también permanecieron intactos.
4. LinuxGSM arrancó el baseline en PID nuevo **76760**, con el hash archivado
   realmente mapeado. Steam y la misma prueba HTTP/cuerpo/contexto pasaron. La
   lista completa de 135 plugins fue idéntica y no hubo errores SourceMod.
5. Al salir, el baseline mostró `Segmentation fault` y produjo un evento systemd
   de señal 11 correspondiente a ese PID. Se conserva el fallo, consistente con
   el ya conocido del baseline; su causa no se volvió a investigar en este ensayo.
6. Se restauró el selector original y LinuxGSM arrancó el DEV habitual, PID
   **78631**, conectado a Steam y con la extensión original mapeada. Se comprobaron
   los cuatro hashes protegidos; se retiraron laboratorio y fixture.

El harness compila sin warnings contra ambos includes y produce el mismo SMX.
Cierra el handle mediante timer diferido en ambas fases, un protocolo común
compatible con el baseline. La fixture recibió exactamente dos peticiones `/text`.
No se requirió jugador ni se repitieron autenticación, identidad, stats o GC.

## Incidencia y atribución

La [auditoría de cores](p5-r7-rollback-srcds/core-audit.json) identifica solo el core
del baseline en la ventana del ensayo y ninguno del candidato. Se extrajeron
metadatos mínimos, sin cores, argumentos ni entorno; los cores originales de
systemd se conservaron. Los mensajes de lifecycle que el colector de salida
incluye en el JSON del baseline pertenecen al log acumulado del candidato:
no se atribuyen como instrumentación del baseline.

Este resultado cierra la aplicación en sitio del ZIP R7 y el arranque/HTTP posterior
al rollback, dentro de T35. No acredita cierre limpio del baseline ni aceptación
global. T08 del candidato mantiene su aceptación con SourceTVSupport y añade otro
quit limpio; restaurar el baseline recupera sus bytes originales sin incorporar
la corrección. La aceptación humana de R6 sigue atribuida al mismo ELF candidato.

## Evidencia conservada

[Resultados](p5-r7-rollback-srcds/results.json),
[candidato](p5-r7-rollback-srcds/candidate-runtime.json),
[aplicación del ZIP](p5-r7-rollback-srcds/applied-rollback.json),
[baseline](p5-r7-rollback-srcds/rollback-runtime.json),
[módulos restaurados](p5-r7-rollback-srcds/rollback-modules.json),
[cierre candidato](p5-r7-rollback-srcds/candidate-shutdown.json),
[cierre baseline](p5-r7-rollback-srcds/rollback-shutdown.json) y
[restauración final](p5-r7-rollback-srcds/cleanup.json).

Evidencia de revisión separada:
`.build/p5-r7-rollback-srcds/review/steamworks-p5-r7-in-place-rollback-srcds-acceptance.zip`,
con manifiesto y checksums. Solo incluye datos filtrados, harness y aplicador;
excluye configuraciones privadas, logs crudos, SDK, módulos del juego y cores.
Los cuatro ZIP P5-R7 conservan el checkpoint previo a este seguimiento. La
[matriz anterior](p5/history/p5-r7-before-in-place-rollback-acceptance-validation-results.json)
queda guardada y la [matriz activa](p5/validation-results.json) concilia T35.
