# Reversión de P5-R18

El paquete nuevo `.build/p5-r18/review/steamworks-p5-r18-competitive-dev-rollback.zip`
se regenera desde una captura por lectura de `competitive_dev`. No se modifica ni
reinicia SRCDS. Se conserva [la captura](p5/r18/baseline-observation.json) y la
[evidencia T26/T35 en sitio de R9](p5-r9-rollback-srcds/results.json).

| Propiedad | Resultado |
|---|---|
| Extensión original | `9ca37aadcd83448d49126d6a72babcb8ec97ec5a06c171b4821ae0ee3cf05d70` |
| Include original | `e51bcd397139edc3d144f130c45f0f8edbf55d12278736e0134e4c650ec64f17` |
| Ausente | `gamedata/steamworks.txt` |
| ZIP rollback | `55153d557f3af7f71f1016c8c394dc74fe29e0cdc9511044c21de29e8bc6dffc` |
| Roundtrip R18 | Candidato→rollback desde ZIP en filesystem desechable; archivos ajenos preservados |
| Aplicación en sitio | Los mismos bytes ZIP se aplicaron en R9; no se repite en R18 |
| Salida del baseline | SIGSEGV conocido conservado; no se acepta cierre limpio |

La captura no contiene CFG, claves, datos de jugadores ni bibliotecas de juego/Steam.
El runtime R18 se limita a extensión/include; helpers, VM y SMX de pruebas quedan
fuera. El ZIP candidato nuevo solo se valida localmente y comparte exactamente
ELF/include con R16 aceptado. Los paquetes anteriores permanecen intactos.

[T25/T29 de R16](last-two-srcds-progress.md) restauró DEV con LinuxGSM, quit limpio
del candidato, mismo conjunto protegido y 1925 CFG intactos. No aplicó el ZIP
rollback. R18 verifica por lectura el mismo DEV antes/después, sin operaciones
de servidor, filtros de red ni cambios de selector. El cierre de la matriz del
perfil Linux x86 no amplía la aceptación del baseline o de otros consumidores.

## Procedimiento para una intervención autorizada

1. Verificar destino, supervisor, manifiesto y `SHA256SUMS`. Contrastar baseline
   real con captura; si cambió, capturar de nuevo antes de intervenir.
2. Detener SRCDS con LinuxGSM y comprobar que terminó. No sustituir este paso
   por unload forzado o desde callbacks activos.
3. Restaurar solo extensión/include originales y retirar `gamedata/steamworks.txt`
   si se creó. Preservar datos, bibliotecas y configuración ajenos.
4. Restaurar selector y exclusivamente las configuraciones cambiadas; conservar
   flags, contraseña y configuración de juego habitual. Verificar hashes/ausencias.
5. Arrancar con el supervisor original y comprobar módulo, consumidores, Steam,
   logs y operaciones afectadas. Ante fallo, detener y conservar evidencia.
6. Retirar solo temporales propios una vez verificada la restauración.

Connect no fue desplegado en esta actualización. Su corrección ya committed se entrega
como parche completo contra c695bf0 en el ZIP de revisión, no como parte del rollback de SteamWorks.
Antes de una futura actualización de ese consumidor se debe capturar su SMX/config
propios; su API pública permanece intacta. Los binarios HTTP antiguos de tres
argumentos deben recompilarse con el contrato de cinco argumentos.

[Checkpoint anterior](p5/history/p5-r17-with-Windows-CI-fixes-p5-rollback.md).
Todo sigue en develop, sin commit, publicación ni despliegue autorizado por este documento.
