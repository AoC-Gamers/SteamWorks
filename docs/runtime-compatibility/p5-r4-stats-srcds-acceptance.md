# P5-R4: stats float positivos y claves inexistentes en SRCDS x86

Fecha local: 2026-11-15. **Pasa la aceptación acotada**: dos recorridos de 733
claves, con las mismas 12 claves float leídas correctamente por cliente y
AccountID, y una clave inexistente rechazada sin modificar las cuatro salidas.
Candidato SHA-256 `bc76b00d0614f8f8489f8d3d30e3d654976f3bfde689f17d8731018d6c7ac639`.
`develop`, sin commit ni producción. El DEV habitual quedó restaurado.

## Entorno y protocolo

El payload runtime se extrajo del ZIP revisado P5-R4 y se verificó contra su
manifiesto. LinuxGSM detuvo temporalmente el servidor habitual y arrancó un árbol
SourceMod aislado en `competitive_dev`, puerto 27015. Solo cambió el selector
`basepath`; se conservaron parámetros, CFG, contraseña y SourceTV habituales.
No se sustituyeron VM, SDK ni bibliotecas del juego.

Se comprobaron los módulos realmente mapeados por dispositivo/inodo y hash,
antes y después, con el mismo PID y extensión ELF32/i386. El perfil fue
`l4d2-23990100-linux-x86`; el debug se activó con máscara 12. El harness nuevo
compiló con SourcePawn 1.12.0.7259 sin warnings. No modifica la API pública ni usa
setters de stats.

El schema local AppID 550 tiene 733 claves de stats y no declara sus tipos
numéricos. El generador conserva únicamente esas claves públicas y comprueba que
la clave negativa `__steamworks_accept_missing_stat_p5_r4__` tampoco aparezca en
stats ni achievements. Los nombres `Avg` y los defaults no se usan como prueba
de tipo: se consulta al proveedor real.

Cada ronda selecciona el único humano autorizado dentro del juego, solicita
stats por cliente/AccountID y espera a leer `Stat.GamesPlayed.Total` por ambas
rutas con valores iguales. Así se evita aprobar claves inexistentes usando un
cache todavía sin cargar. En cada timer se revalidan userid y AccountID; el
harness cancela al salir el cliente, cambiar de mapa o descargarse.

## Resultado

El recorrido se reparte en lotes de 16 claves por timer. Para cada clave se
llama a los dos getters float. Un éxito requiere ambos retornos positivos,
valores finitos y bits idénticos, sobrescribiendo los sentinels NaN iniciales.
«Positivo» describe el éxito de lectura, no exige un valor numérico mayor que cero.
No se imprimen valores personales ni identificadores.

| Comprobación | Ronda 1 | Ronda 2 |
|---|---:|---:|
| Claves del schema recorridas | 733 | 733 |
| Claves float leídas con éxito por ambas rutas | 12 | 12 |
| Discrepancias o valores no finitos | 0 | 0 |
| Fallos de conservación de salidas | 0 | 0 |
| Comprobaciones de clave inexistente, antes/después | 2 | 2 |

Las 12 claves coinciden entre rondas:

- `Stat.KitsUsed.Avg`, `Stat.PillsUsed.Avg`, `Stat.KitsShared.Avg`
- `Stat.WasRevived.Avg`, `Stat.WasProtected.Avg`, `Stat.FFDamageGame.Avg`
- `Stat.PillsShared.Avg`, `Stat.TeamRevived.Avg`, `Stat.TeamProtected.Avg`
- `Stat.AdrenalineShared.Avg`, `Stat.AdrenalineUsed.Avg`, `Stat.DefibrillatorsUsed.Avg`

Las otras 721 claves rechazaron el getter float por ambas rutas y conservaron
sus sentinels. No se infiere que todas sean enteras: pueden existir stats no
disponibles para esa cuenta.

La clave inexistente se consultó como entero y float por cliente y por AccountID:
16 lecturas individuales entre ambas rondas, todas con retorno false. Se
conservaron dos sentinels enteros distintos y los bits exactos de dos payloads
NaN float distintos. Traza representativa:

```text
[SW-STATS] integer_control pass=1
[SW-STATS] float key=Stat.KitsUsed.Avg client=1 account=1 finite=1 matches=1 pass=1
[SW-STATS] missing pass=1 clientInt=0 accountInt=0 clientFloat=0 accountFloat=0 intPreserved=1 floatBitsPreserved=1
[SW-STATS] complete schema_keys=733 float_keys=12 failures=0 pass=1
```

`client` y `account` son booleanos de éxito, no identificadores. No se escribieron
stats ni se inspeccionaron tickets. No hubo logs de errores SourceMod ni marcadores
de crash. Los asserts no se debilitaron para obtener el resultado.

## Cierre, evidencia y alcance

El cierre explícito por `quit` observó consumidores detenidos. Se detuvo después
el wrapper LinuxGSM y se restauró byte a byte el CFG original. El DEV habitual
volvió con `addons/sourcemod1`, extensión original mapeada (`9ca37aad…`), conexión
a Steam y hashes originales de selector/AoC/Family Share. Se retiró el árbol
seleccionado para el laboratorio y sus copias privadas.

[Resultados y hashes](p5-r4-stats-srcds/results.json),
[primera ronda](p5-r4-stats-srcds/runtime.json),
[segunda ronda](p5-r4-stats-srcds/runtime-repeat.json),
[comprobaciones](p5-r4-stats-srcds/checks.json),
[módulos mapeados](p5-r4-stats-srcds/loaded-modules-final.json),
[cierre](p5-r4-stats-srcds/shutdown.json),
[restauración](p5-r4-stats-srcds/cleanup.json) y
[harness](p5-r4-stats-srcds/harness.sp) conservan el ensayo. La receta de compilación
está en `tests/runtime/README.md`; el include generado y hash del schema están en
la evidencia. El ZIP separado se guarda en `.build/p5-r4-stats-srcds/review/`.

T03 incorpora esta aceptación junto con las lecturas enteras y de tipo incorrecto
anteriores del mismo binario. No se aprueba P5 globalmente: siguen pendientes
consumidores/forwards completos, GC, negativos de identidad, ciclo de vida restante,
operación sostenida y reversión en sitio desde el ZIP. SRCDS no estuvo
instrumentado con ASan. Los ZIP P5-R4 y las evidencias anteriores permanecen intactos.
