# Reversión de P5-R20

La captura de archivos propios de SteamWorks se renueva por lectura desde
`competitive_dev`, sin iniciar, detener o modificar servidores. El conjunto está
en `.build/p5-r20/review/`; sus ZIP nuevos se validan en filesystem desechable.

| Propiedad | Resultado |
|---|---|
| Extensión habitual | `9ca37aadcd83448d49126d6a72babcb8ec97ec5a06c171b4821ae0ee3cf05d70` |
| Include habitual | `e51bcd397139edc3d144f130c45f0f8edbf55d12278736e0134e4c650ec64f17` |
| Gamedata | `gamedata/steamworks.txt` ausente, retirada explícita al restaurar |
| ZIP rollback R20 | `55153d557f3af7f71f1016c8c394dc74fe29e0cdc9511044c21de29e8bc6dffc` |
| Bytes ya aplicados | Exactamente el ZIP usado en T35 R19, también idéntico al de R9 |
| Roundtrip R20 | ZIP candidato→ZIP rollback local, hashes/ausencias y CFG/plugin ajenos preservados |
| Ensayo runtime incorporado | R19, mismo árbol detenido, un observador y SourceTVSupport; Steam/HTTP/cierre pasan |
| R20 en sitio | No instalado ni aplicado de nuevo en SRCDS |
| Cierre del baseline | Quit limpio observado en el perfil mínimo R19; SIGSEGV histórico R9 no cerrado globalmente |

El candidato R20 contiene exactamente el ELF/include aceptados por revisión 12
y por el [seguimiento T26/T35 R19](p5-r19-rollback-srcds-acceptance.md). El nombre
nuevo y la documentación actualizada no equivalen a otra instalación en sitio.
El contrato runtime del candidato sigue limitado a dos archivos; rollback no
incluye CFG privadas, SDK, VM, helpers, SMX o datos de jugadores.

Se preservan la [aplicación real R19](p5-r19-rollback-srcds/applied-rollback.json),
[restauración DEV/1843 CFG](p5-r19-rollback-srcds/cleanup.json), la
[captura R20](p5/r20/baseline-observation.json) y el
[roundtrip R20](p5/r20/rollback-roundtrip.json). No se reinterpreta el fallo de
salida histórico ni se atribuye al baseline todo el comportamiento del candidato.
GC remoto positivo sigue pendiente y no hay aceptación global nueva.

La revisión final exige que el resultado local se contraste con
`p5/r20/rollback-roundtrip.json`. En la matriz, la aplicación anterior está en
`prior_in_place_evidence`; `current_in_place_evidence` es null porque R20 no
aplicó el ZIP en un servidor. La identidad de bytes conserva la evidencia
R19 sin convertirla en una nueva intervención.

## Procedimiento para una futura intervención autorizada

1. Contrastar destino, supervisor, manifiesto, `SHA256SUMS` y baseline real; si
   cambió, renovar la captura propia antes de intervenir.
2. Detener SRCDS mediante LinuxGSM y comprobar que terminó. No forzar unload
   desde callbacks ni sustituir un ELF que siga mapeado.
3. Restaurar únicamente extensión/include desde el ZIP y la ausencia de gamedata
   declarada. Preservar datos, bibliotecas, plugins y CFG ajenos.
4. Restaurar selector y solo las configuraciones que se hubieran cambiado;
   conservar flags/configuración habituales. Verificar hashes y ausencias.
5. Arrancar con LinuxGSM y comprobar módulo, Steam, consumidores y operaciones
   afectadas; conservar evidencia ante fallos.
6. Retirar exclusivamente los temporales propios después de verificar el servicio.

Connect no se instala ni revierte en esta actualización. Requiere su propia
captura SMX/config antes de una intervención; el parche completo incluido en
source-review no autoriza aplicarlo. Los consumidores HTTP de tres argumentos
siguen requiriendo recompilación.

[Estado R19 anterior](p5/history/p5-r19-with-T26-T35-acceptance-p5-rollback.md).
Todo queda en develop, sin commit, push ni despliegue. Los ZIP anteriores se conservan.
