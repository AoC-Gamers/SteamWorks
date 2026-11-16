# Fixtures protegidos

Estas son entradas reproducibles, no resultados nuevos. Los bytes se preservan
con `tests/fixtures/** -text` en `.gitattributes`.

| Entrada | Procedencia y consumidor |
|---|---|
| `api/api-inventory-p0.json` | Inventario P0 original; comparación completa de contrato por integración |
| `stats/GetSchemaForGame_550.json` | Schema histórico AppID 550; generador de claves y harness stats |
| `gameserver014-prefix.json` | Prefijo SDK 1.55 independiente; orden de slots auditado frente al adaptador |
| `sourcepawn/auth_forward_receiver.sp` | Fuente compilada para marshalling y agregación con VM real |
| `runtime-profiles/linux-x86-candidate.json` | Perfil documental bloqueado; evidencia SDK y rechazo de activación |
| `runtime-profiles/competitive-dev-linux-x86.json` | Perfil documental del selector nativo, deshabilitado como JSON |
| `runtime-profiles/modules/` | Metadatos SDK 1.55 Linux x86 y 1.58a Linux/Windows x86, consumidos por tests |
| `runtime-profiles/competitive-dev/` | Metadatos de motor/runtime y traza mínima; hashes verificados transitivamente |
| `sdk-provenance/sdk-toolchain.json` | Procedencia enlazada por el perfil documental, sin copiar SDK |

Las entradas trasladadas proceden del commit `295e274de754fb65835188f115a2fd4dd917e289`.
El perfil candidato solo cambia su referencia relativa de procedencia SDK a
`../sdk-provenance/sdk-toolchain.json`: no cambia evidencia, versiones ni flags. Los archivos
referidos por hash mantienen bytes exactos. La traza preserva semántica observada;
no es un log para eliminar por extensión. No regenerar baseline con la API actual.

Los perfiles no habilitan el producto; el selector real está en C++. No editar
`enabled` para simular aceptación ni inferir soporte Windows por exports PE.
Una recepción GC positiva remota requiere evidencia externa todavía pendiente.

`manifest.json` fija SHA256 y procedencia de las entradas conservadas; el test
documental los comprueba. No regenerarlo para ocultar un cambio de fixture.
