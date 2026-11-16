# Cobertura y límites

Esta matriz describe comportamientos y procedencia. No es una colección de logs
ni una ejecución nueva; los resultados actuales se guardan fuera de Git.

| Contrato | Suite/escenario | Nivel | Evidencia runtime histórica | Límite |
|---|---|---|---|---|
| API y firmas/opcionalidad | Fixture P0 + integración | Inventario local | No necesaria para comparar firmas | No prueba ABI/runtime |
| Stats: fallo conserva salida, identidad y AppID | stats-natives; harness stats | Dobles y SRCDS | 2026-11-15; Linux x86; F2; R3/R4, usuario autorizado | Solicitud aceptada no significa lectura disponible |
| Identidad: capacidad/NUL y preautorización | steamid-string; harness identity/consumer | Dobles y SRCDS | 2026-11-15/16; Linux x86; F4; T15/T25/T29 | No inferir grupo/perfil desde identidad |
| Auth 014, ticket y agregación | auth-binding/auth-forward | SourceHook/VM reales offline y SRCDS | 2026-11-15; Linux x86; F1/F2; adaptador y autenticación | Proveedor offline no emite ticket real |
| Init/teardown con SourceTVSupport | detours/init-coexistence/resource-lifecycle | Dobles/x86 y SRCDS | 2026-11-15; Linux x86; F3; T08 en mismo proceso | No toda la pila de consumidores |
| Streaming: dos celdas y payload | http-stream-context | Wire independiente y SRCDS | 2026-11-15; Linux x86; F2; cuatro tandas, una tras mapa | No todo endpoint remoto |
| HTTP estricto y GC snapshot | typed-boundary/resource-lifecycle, negativos mínimos | SDK simulado y SRCDS | 2026-11-16; Linux x86; F5; HTTP real, GC vacío/proveedor local | GC remoto positivo pendiente |
| Cero/uno/varios forwards | auth-forward; harness forward | VM real y SRCDS | 2026-11-15; Linux x86; F4; T23 | ET_Event no propaga -1 individual como agregado |
| Capacidades e interfaces | binary-evidence/typed-boundary; harness capability | Metadata/registro y SRCDS | 2026-11-15; Linux x86; F4; T11, GC opcional | Exports no son aceptación Windows |
| Propietario HTTP pendiente y carga tardía | resource-lifecycle; harness pending/lifecycle | Dobles y SRCDS | 2026-11-15; Linux x86; F4; T20 y T16 | Descarga forzada con dependientes no soportada |
| Instalación y reversión real | review_artifacts; harness rollback | Filesystem y SRCDS | 2026-11-16; Linux x86; F5; T26/T35 R19 en árbol detenido | Runtime mínimo, no conjunto completo |
| Destrucción VM | sourcepawn-environment | VM real ASan offline | No se instala en servidor | No acredita VM distribuida |
| Paquete limpio y rutas documentales | Python/documentación y staging/CI | Offline | No requiere SRCDS | Windows runtime pendiente |

## Atribución histórica

Las fuentes siguientes están fijadas al commit
`295e274de754fb65835188f115a2fd4dd917e289`; las fechas, revisiones de compilación
y hashes completos de cada ensayo están en sus informes. No trasladar una
observación entre ELFs por compartir número de revisión documental.

- [Integración consolidada y fechas por suite](https://github.com/AoC-Gamers/SteamWorks/blob/295e274de754fb65835188f115a2fd4dd917e289/docs/runtime-compatibility/p5-integration.md)
- [Matriz con revisión, fechas y hashes por escenario](https://github.com/AoC-Gamers/SteamWorks/blob/295e274de754fb65835188f115a2fd4dd917e289/docs/runtime-compatibility/p5/validation-results.json)
- [Autenticación/identidad/stats](https://github.com/AoC-Gamers/SteamWorks/blob/295e274de754fb65835188f115a2fd4dd917e289/docs/runtime-compatibility/p5-r3-auth-srcds-acceptance.md)
- [Floats y claves inexistentes](https://github.com/AoC-Gamers/SteamWorks/blob/295e274de754fb65835188f115a2fd4dd917e289/docs/runtime-compatibility/p5-r4-stats-srcds-acceptance.md)
- [Buffers HTTP/GC, 2026-11-16, ELF 72664c65…](https://github.com/AoC-Gamers/SteamWorks/blob/295e274de754fb65835188f115a2fd4dd917e289/docs/runtime-compatibility/review12-srcds-acceptance.md)
- [Instalación/reversión R19, 2026-11-16, mismo ELF](https://github.com/AoC-Gamers/SteamWorks/blob/295e274de754fb65835188f115a2fd4dd917e289/docs/runtime-compatibility/p5-r19-rollback-srcds-acceptance.md)

Los identificadores F1–F5 distinguen los payloads observados, no niveles de
soporte. Las revisiones/capturas de fuentes originales se consultan en la matriz
inmutable anterior; `295e274` fija la evidencia conservada, **no afirma que todos
los ELFs se compilaran desde ese checkout limpio**.

| Payload | SHA256 del ELF histórico |
|---|---|
| F1 | `ddfdf9bf2f9bde625719ba171c59f7cac8b56cbe136a15e4911c7acbf00383ad` |
| F2 | `bc76b00d0614f8f8489f8d3d30e3d654976f3bfde689f17d8731018d6c7ac639` |
| F3 | `82347b99d5130bd411cdc0b35d215dc87fc82d394a2252ccd83d5d4940b6ca5a` |
| F4 | `839ff0751b04141031bdc6dffc48662d0804f52227f2c91015bd87c6b84cf192` |
| F5 | `72664c658f6d28e78a975ec98c96879dc641f96725206f514f159ea17125fcb3` |

El payload final histórico Linux x86 tiene SHA256
`72664c658f6d28e78a975ec98c96879dc641f96725206f514f159ea17125fcb3`;
el include, `969541624a2ba2f14aa07e040f96e6a50db40503baa21cba6206e0b1bd25aad8`.
Ensayos anteriores sobre `839ff075…` mantienen su propia atribución en la matriz.
Las 15 suites y build de R20 no equivalen a reinstalar su ZIP nuevo. El quit
limpio del perfil mínimo no declara resuelto el SIGSEGV histórico del baseline.
GC remoto positivo y runtime Windows permanecen pendientes.
