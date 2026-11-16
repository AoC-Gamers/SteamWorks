# Reversión de P5-R19

Se renueva por lectura la captura de los archivos propios de SteamWorks en
`competitive_dev`, en la fase de empaquetado. El ZIP
`.build/p5-r19/review/steamworks-p5-r19-competitive-dev-rollback.zip` se contrasta
con el candidato real en un filesystem desechable.

| Propiedad | Resultado |
|---|---|
| Extensión habitual | `9ca37aadcd83448d49126d6a72babcb8ec97ec5a06c171b4821ae0ee3cf05d70` |
| Include habitual | `e51bcd397139edc3d144f130c45f0f8edbf55d12278736e0134e4c650ec64f17` |
| Gamedata | `gamedata/steamworks.txt` ausente, ausencia restaurada |
| SHA-256 ZIP rollback | `55153d557f3af7f71f1016c8c394dc74fe29e0cdc9511044c21de29e8bc6dffc` |
| Roundtrip R19 | ZIP candidato→ZIP rollback; hashes/ausencias/CFG ajenos verificados localmente |
| Ensayo en sitio | Aplicado desde ZIP en el mismo árbol aislado en el seguimiento T35 R19 |
| Salida del baseline | SIGSEGV histórico R9; quit limpio observado en el perfil mínimo R19, sin cierre general del fallo |

El candidato R19 contiene el ELF `72664c65…` aceptado con alcance GC/HTTP en la
revisión 12. El seguimiento [T26/T35](p5-r19-rollback-srcds-acceptance.md) instala el ZIP local
exacto y aplica el rollback en el mismo árbol detenido. No se atribuye esta prueba
a los ZIP generados por CI. El ensayo de revisión 12 anterior restauró archivos
y selector con LinuxGSM; por sí solo no era una aplicación del ZIP R19. Se conserva por
separado [esa restauración](review12-srcds/cleanup.json), la
[captura actual](p5/r19/baseline-observation.json), el
[roundtrip actual](p5/r19/rollback-roundtrip.json) y la
[aplicación en sitio R9](p5-r9-rollback-srcds/results.json).

El rollback incluye únicamente extensión/include habituales, sin CFG, SDK,
bibliotecas de juego, VM, helpers, SMX o datos de jugadores. La ausencia de
SteamWorks gamedata se restaura explícitamente; no se borra configuración ajena.
No restaura cambios de Connect: ese consumidor necesita su propia captura
SMX/config antes de cualquier intervención autorizada. Los binarios HTTP de tres
argumentos siguen siendo incompatibles con el contrato de cinco argumentos.

## Procedimiento para una intervención futura autorizada

1. Contrastar destino, supervisor, manifiesto, `SHA256SUMS` y baseline real. Si
   cambió, capturar otra vez los archivos propios antes de intervenir.
2. Detener SRCDS mediante LinuxGSM y comprobar que terminó; no forzar unload
   dentro de callbacks ni sustituir bytes de un ELF que siga mapeado.
3. Restaurar únicamente extensión/include del ZIP y la ausencia de gamedata
   registrada. Preservar datos, bibliotecas y configuración ajenos.
4. Restaurar selector y solo las configuraciones que se hubieran cambiado;
   conservar flags y configuración de juego habituales. Verificar hashes.
5. Arrancar con LinuxGSM y comprobar módulo, Steam, consumidores y operaciones
   afectadas; conservar evidencia ante fallos.
6. Retirar exclusivamente los temporales propios tras verificar la restauración.

El empaquetado R19 probó la reversión local; el seguimiento T35 ya la aplica en
SRCDS aislado con un observador mínimo. DEV queda restaurado, cuatro hashes y
1843 CFG intactos. No se repite la pila completa de plugins ni se cierra GC remoto.
Este seguimiento queda sin commit, publicación ni despliegue en producción. [Estado anterior](p5/history/p5-r18-before-review12-integration-p5-rollback.md).
