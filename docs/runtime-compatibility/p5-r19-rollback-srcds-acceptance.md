# P5-R19: T26 y T35 en SRCDS x86

2026-11-16. **T26/T35 pasan en el alcance ejecutado:** instalación del ZIP
candidato local exacto, reversión desde ZIP en el mismo árbol detenido y
arranque/Steam/HTTP/cierre de ambos. Solo se cargó el observador común de prueba,
con las extensiones habituales y SourceTVSupport. No se valida aquí la pila
completa de consumidores ni se recibe contenido GC remoto.

`develop`, fuentes de `adffed0cc90cad929183b7adc896a6f86ce281da` sin cambios de
producto/API. Este seguimiento queda sin commit, push ni producción. El CI verde
previo de ese commit no se presenta como aceptación runtime de sus assets:
**el ZIP seleccionado es el candidato local de P5-R19**, no el alias mutable
`channel/develop` ni un binario suelto del build.

## Archivos fijados antes de intervenir

| Elemento | SHA-256 |
|---|---|
| ZIP candidato P5-R19 | `b8a4a8cd20b15572a3e2a31e7be5235d761ac7b1f0e566c05cef57939f893f9d` |
| Extensión instalada | `72664c658f6d28e78a975ec98c96879dc641f96725206f514f159ea17125fcb3` |
| Include instalado | `969541624a2ba2f14aa07e040f96e6a50db40503baa21cba6206e0b1bd25aad8` |
| ZIP rollback P5-R19 | `55153d557f3af7f71f1016c8c394dc74fe29e0cdc9511044c21de29e8bc6dffc` |
| Extensión restaurada | `9ca37aadcd83448d49126d6a72babcb8ec97ec5a06c171b4821ae0ee3cf05d70` |
| Include restaurado | `e51bcd397139edc3d144f130c45f0f8edbf55d12278736e0134e4c650ec64f17` |

Se verificaron manifiesto, checksums y todos los miembros de los ZIP antes de
operar y otra vez dentro del guest. Los paquetes existentes permanecen intactos.
No se instalaron SDK, VM diagnóstica, bibliotecas Steam/engine ni helpers nativos.
El mismo harness compiló contra ambos includes sin warnings y produjo SMX idéntico.

## Secuencia observada

1. LinuxGSM `/app/l4d2server` detuvo el DEV habitual. Se creó la copia
   `addons/sourcemod_acceptance`, seleccionada temporalmente en el CFG LinuxGSM,
   en `competitive_dev:27015`, conservando flags/CFG habituales y sin contraseña
   añadida. Los plugins de producto quedaron fuera de la copia; no se movieron
   ni modificaron sus originales. Solo allí se desactivó la actualización
   automática de SourceMod.
2. Con SRCDS detenido, el instalador abrió el ZIP candidato en el contenedor e
   instaló únicamente sus dos miembros runtime: extensión/include. Se contrastó
   previamente el baseline real con los bytes del rollback y su gamedata ausente.
3. LinuxGSM arrancó el candidato, PID **44301**. El módulo mapeado coincide en
   dispositivo/inodo/hash. Steam conectado; una petición HTTP loopback completó
   con estado 200, cuerpo de 16 bytes, ambas celdas de contexto correctas y cierre
   diferido del handle. Una fila de plugin, sin errores. Quit limpio.
4. Sin cambiar el selector ni el árbol, y tras comprobar que no quedaba SRCDS,
   se abrió el ZIP rollback dentro del guest y se aplicaron sus bytes. Se retiró
   el gamedata testigo creado **solo después de detener el candidato**: no lo
   generó el producto ni llegó a cargarse. Se preservaron core.cfg y el CFG testigo
   ajeno de laboratorio, los cuatro archivos globales protegidos y el árbol CFG.
5. LinuxGSM arrancó el baseline, PID **46126**, con los bytes originales realmente
   mapeados. Repitió Steam/HTTP/cuerpo/contexto/callback/cierre sin fallos, con la
   misma fila del observador. Su quit tampoco mostró marcadores de crash en este
   perfil mínimo. No hubo errores SourceMod en ninguna de las dos fases.
6. Se restauró byte por byte el selector original y LinuxGSM arrancó el DEV
   habitual, PID **48073**, conectado a Steam. Sus cuatro hashes protegidos y las
   **1843 configuraciones** coinciden con la captura previa. Se retiraron
   laboratorio, recovery y fixture HTTP; solo este último escuchaba en loopback.

El fixture contabilizó exactamente dos peticiones `/text`, una por proceso.
El protocolo común comprueba la lectura raw completa y cierra por timer para
ser compatible con el baseline antiguo. No se usa el helper textual nuevo para
atribuir al baseline el contrato corregido de REV-HTTP-02.

## Cierre, incidencias y límites

El SIGSEGV del baseline documentado en R9 **no se reprodujo en este perfil mínimo**.
No se infiere su solución general, ni se reinterpreta la traza histórica como un
cierre limpio. Journal/coredumpctl no mostraron nuevos eventos SRCDS desde el
inicio del ensayo y las consolas de ambos procesos no tenían marcadores de crash.
No se cambiaron límites de cores ni se suprimieron comprobaciones; no se leyeron,
exportaron o eliminaron cores. El fallo histórico conserva su alcance original.

El primer sondeo del DEV restaurado agotó su ventana de 50 segundos antes de
conectarse a Steam. Se mantuvo recovery y se repitieron las mismas comprobaciones
cuando apareció la conexión, sin otro reinicio ni cambios de CFG. La restauración
solo se dio por terminada después de verificar conexión, módulos y todos los hashes.

T26 acredita instalación y arranque del payload exacto del ZIP; T35, aplicación
real del rollback en el mismo árbol y operación mínima posterior. No se repiten
el conjunto completo de consumidores, autenticación/identidad/stats humanos,
carga prolongada o recuperación de red. **GC remoto positivo sigue pendiente**;
no se declara aceptación global nueva ni de los ZIP generados por CI.

Se contrastaron **157 fuentes** de producto/pruebas y **165 archivos de artefactos
anteriores**, sin cambios. No se exportaron CFG privadas, argv, entorno, tickets,
identidades, logs completos ni binarios del juego. Los resultados y ZIP históricos
se conservan; los paquetes P5-R19 no se regeneran en este seguimiento.

## Evidencia

[Resultados y hashes](p5-r19-rollback-srcds/results.json),
[instalación real](p5-r19-rollback-srcds/candidate-installation.json),
[candidato](p5-r19-rollback-srcds/candidate-runtime.json),
[módulos candidato](p5-r19-rollback-srcds/candidate-modules.json),
[rollback aplicado](p5-r19-rollback-srcds/applied-rollback.json),
[baseline](p5-r19-rollback-srcds/rollback-runtime.json),
[módulos baseline](p5-r19-rollback-srcds/rollback-modules.json),
[cierre candidato](p5-r19-rollback-srcds/candidate-shutdown.json),
[cierre baseline](p5-r19-rollback-srcds/rollback-shutdown.json),
[cores](p5-r19-rollback-srcds/core-events.json) y
[DEV restaurado](p5-r19-rollback-srcds/cleanup.json).

[La matriz anterior](p5/history/p5-r19-before-T26-T35-srcds-validation-results.json)
permanece como snapshot previo; la matriz activa añade únicamente este alcance.
Los comandos adjuntos son registros ligados a este SHA, instalación y recovery
privado, no un instalador general ni autorización para operar producción.
Paquete separado:
`.build/p5-r19-rollback-srcds/review/steamworks-p5-r19-install-in-place-rollback-srcds-acceptance.zip`.
