# P5-R5: reversión en sitio y arranque del baseline en SRCDS x86

Fecha local: 2026-11-15. **La restauración desde el ZIP y el arranque del baseline
pasan en el árbol aislado. El cierre limpio del proceso no pasa:** se observó
`SIGSEGV` al salir tanto del candidato como del baseline archivado. T35 incorpora
la restauración/arranque; T08 registra el fallo de salida sin resolver. P5 no tiene
aceptación global. `develop`, sin commit ni producción.

## Contrato probado

| Estado | Extensión SHA-256 |
|---|---|
| Candidato P5-R5 | `bc76b00d0614f8f8489f8d3d30e3d654976f3bfde689f17d8731018d6c7ac639` |
| Baseline del ZIP de reversión | `9ca37aadcd83448d49126d6a72babcb8ec97ec5a06c171b4821ae0ee3cf05d70` |

El include restaurado tiene SHA-256
`e51bcd397139edc3d144f130c45f0f8edbf55d12278736e0134e4c650ec64f17`.
El ZIP de reversión tiene SHA-256
`55153d557f3af7f71f1016c8c394dc74fe29e0cdc9511044c21de29e8bc6dffc`.
Todos se verificaron contra el manifiesto P5-R5, sin regenerar los archivos.

Se usó `competitive_dev`, LinuxGSM `/app/l4d2server`, puerto 27015 y el mismo
`addons/sourcemod_acceptance` durante candidato y rollback. La restauración no se
simuló cambiando el selector al servidor original: el ZIP se abrió dentro del
contenedor y sus bytes sobrescribieron binario/include de ese árbol detenido.

## Ejecución y resultados

1. Se detuvo el DEV habitual con LinuxGSM y se guardó privadamente su selector.
   Se copió el árbol SourceMod y sus datos; solo allí se instaló el payload del
   ZIP candidato y el harness. Parámetros, contraseña, SourceTV y CFG habituales
   se conservaron. El actualizador SourceMod de la copia quedó desactivado.
2. El candidato arrancó, se conectó a Steam, listó 135 plugins sin filas Error/Failed
   y pasó una petición HTTP loopback: HTTP 200, 16 bytes esperados y contexto
   sintético completo. El handle se cerró diferido, para usar la misma práctica
   compatible con el baseline antiguo. El harness compila sin warnings contra
   ambos includes.
3. Se pidió `quit`. Aparecieron `plugin_end completed=1 failed=0` y consumidores
   detenidos, pero después `Segmentation fault (core dumped)`. La comprobación de
   salida limpia falló. LinuxGSM se detuvo y el aplicador verificó por separado
   que no quedaba ningún SRCDS antes de escribir archivos.
4. Se validó el ZIP de reversión, sus rutas permitidas y cada hash antes de
   restaurar. Se recuperaron los bytes originales de extensión/include y se
   retiró `gamedata/steamworks.txt`, cuyo estado anterior era ausente. Para probar
   esa retirada se creó un archivo testigo después de detener el candidato;
   **no fue un archivo generado por el candidato ni se cargó en el juego**.
5. El selector aislado permaneció idéntico. `configs/core.cfg` y una configuración
   testigo ajena al conjunto restaurado conservaron sus hashes. Los archivos
   originales del DEV también permanecieron intactos.
6. LinuxGSM arrancó un PID nuevo en el mismo árbol. Se comprobó por dispositivo,
   inodo y hash que mapeaba el baseline archivado. Steam conectado; la lista de
   135 plugins fue idéntica; HTTP, cuerpo y contexto volvieron a pasar. No hubo
   logs de errores SourceMod ni marcadores de crash durante la operación.
7. Al pedir `quit` al baseline también apareció `SIGSEGV` después de `plugin_end`.
   Se guardó esta incidencia, sin declarar su cierre limpio ni excluirla del informe.

La fixture registró exactamente dos peticiones `/text`, una por cada arranque.
No se necesitó un jugador ni se probaron autenticación/stats/GC del baseline.
No se sustituyeron la VM instalada ni bibliotecas Steam/engine para hacer pasar
la prueba. El rollback recupera el comportamiento anterior, no las correcciones
nuevas del candidato.

## Incidencia comparativa de cierre

Hay dos cores systemd, correspondientes a ambos procesos. Se exportaron solo
para inspección privada temporal, con argumentos de frames omitidos y sin
redescarga de símbolos. Las versiones de extensión usadas para símbolos se
ajustaron únicamente en el laboratorio detenido y se restauraron inmediatamente
al hash del baseline. Las copias temporales de cores fueron retiradas; los cores
originales de systemd se conservan y no se incluyen en el paquete de evidencia.

Las [trazas filtradas](p5-r5-rollback-srcds/shutdown-core-stacks.json) muestran en
ambos casos un hilo con frames de `/data/.steam/sdk32/steamclient.so` y una llamada
por manejador de señal sin resolver. Esto **no identifica por sí solo la causa**
ni permite atribuir el fallo a SteamWorks o a un componente concreto. Requiere
investigación de salida/lifetime; no se corrigió ni se suprimieron comprobaciones.
Los mensajes de shutdown no equivalen a terminación limpia.

## Restauración del DEV habitual

Después de detener el wrapper se restauró byte a byte el selector original y se
arrancó el DEV habitual con LinuxGSM. Se verificaron `addons/sourcemod1`, la
extensión original mapeada, Steam conectado y hashes originales de selector,
extensión y CFG de AoC/Family Share. Se retiraron árbol aislado, copias privadas,
archivo testigo y fixture. El servidor habitual quedó funcionando y producción
no fue modificada.

## Evidencia y alcance

[Resultados/hashes](p5-r5-rollback-srcds/results.json),
[candidato](p5-r5-rollback-srcds/candidate-runtime.json),
[aplicación real del ZIP](p5-r5-rollback-srcds/applied-rollback.json),
[baseline arrancado](p5-r5-rollback-srcds/rollback-runtime.json),
[módulos restaurados mapeados](p5-r5-rollback-srcds/rollback-modules.json),
[cierre candidato](p5-r5-rollback-srcds/candidate-shutdown.json),
[cierre baseline](p5-r5-rollback-srcds/rollback-shutdown.json) y
[restauración final](p5-r5-rollback-srcds/cleanup.json) permiten revisar cada fase.
El ZIP de evidencia separado está en `.build/p5-r5-rollback-srcds/review/`.

La API pública del repositorio se conserva. T35 acepta solo la reversión en sitio,
coherencia de archivos y arranque/operación mínima del baseline en esta copia.
T08 permanece fallido por el incidente de salida; tampoco se aprueban todos los
consumidores, gameplay, GC ni operación sostenida. Los ZIP P5-R5 anteriores siguen
intactos y reflejan su checkpoint previo; la matriz actual incorpora este seguimiento.
