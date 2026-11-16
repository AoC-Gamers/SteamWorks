# P5-R7: integración con aceptación humana de P5-R6

Rama `develop`, base `46e3ee05fc29ad090a1d6f7b1cb5e3dca9855350`.
La reconstrucción Linux i386 reproduce exactamente el ELF aceptado en T08 y
P5-R6: `82347b99d5130bd411cdc0b35d215dc87fc82d394a2252ccd83d5d4940b6ca5a`.
La API pública se conserva. Esta revisión renueva pruebas, documentación y paquetes
locales. El seguimiento autorizado añade la aceptación de reversión en sitio en
`competitive_dev`, LinuxGSM y una copia aislada en 27015. Sin publicación ni
producción. **P5 no tiene aceptación global.**

## Evidencia conciliada

| Evidencia del mismo ELF | Resultado aceptado en SRCDS x86 |
|---|---|
| [T08 con SourceTVSupport](t08-coexist-srcds-acceptance.md) | Dos reinicializaciones en un mismo PID, un forward de conexión por ciclo, 25 completions HTTP, 18 bytes streaming con contexto completo, respuesta antigua retenida sin callback obsoleto y quit sin SIGSEGV/nuevos cores |
| [Autenticación e identidad P5-R6](p5-r6-auth-srcds-acceptance.md) | Payload del ZIP revisado, un humano autorizado, un evento 014, despacho SourceMod 0 y validación Steam 0 por separado; un evento en cada forward público; identidad/buffers/licencia correctos |
| [Reversión en sitio P5-R7](p5-r7-rollback-srcds-acceptance.md) | ZIP real aplicado al mismo árbol detenido; baseline con hashes originales, Steam y HTTP correctos; candidato sale limpio, baseline conserva SIGSEGV al salir |
| [Stats P5-R6](p5-r6-auth-srcds/results.json) | Entero igual por cliente/AccountID, clave entera como float rechazada con sentinels intactos; dos recorridos de 733 claves, las mismas 12 floats finitas coincidentes y 16 lecturas de clave ausente rechazadas preservando las cuatro salidas |

Los ensayos autorizados usaron LinuxGSM y una copia SourceMod aislada en 27015,
con los flags/configs habituales y sin añadir contraseña. Terminaron con el DEV
original conectado a Steam, selector/módulo/hashes protegidos restaurados y
laboratorio retirado. No se registraron tickets, AccountIDs del forward ni valores
personales de stats; no hubo escrituras de stats. La VM instalada no se reemplazó.

T01–T03, T08/T12, HTTP y el payload instalado desde ZIP cuentan con aceptación en
el alcance de esos ensayos. T36 conserva las pruebas binarias sintéticas y añade
recepción real en P5-R6. Identidad, consumidores, forwards y lifecycle mantienen
sus variantes todavía no ejecutadas. Un float «exitoso» significa lectura válida,
finita e igual por ambas rutas; no exige un valor mayor que cero.

La aceptación anterior de `bc76b00d…` y el fallo de reinicialización de
`cd42f40c…` conservan su atribución. No se utilizan como prueba de un binario
diferente. R7 sí conserva el mismo SHA de los dos ensayos actuales; solo renueva
paquetes/documentación y validación local. Después se instaló su ZIP y se aplicó
la reversión en sitio; el seguimiento se conserva separado de los ZIP originales.
La [matriz anterior con seguimiento humano](p5/history/p5-r6-with-auth-followup-validation-results.json)
y todos los paquetes previos permanecen intactos.

## Validación integrada repetida

- Build x86 completo con warnings como errores; mismo SHA que el módulo mapeado
  en ambas aceptaciones. SDK 1.58a intacto: 55 headers/dependencias contra P0.
- **14 suites x86 pasan**, con comparación de declaraciones, registros, enums,
  typesets y opcionalidad de la API pública.
- Convivencia: 142 comprobaciones de código x86 ejecutable, 21 de admisión del
  llamador y control negativo. Wrappers/rollback: 27 escenarios.
- Binding 014 con SourceHook: 18 comprobaciones y control negativo 015.
  CForward/VM: 227 comprobaciones y 16 ciclos de entorno. Streaming: 92
  comprobaciones y control negativo. ASan/LSan/UBSan activos sin supresiones,
  incluidos lifecycle, streaming y convivencia nativos.
- Tres consumidores recompilados sin warnings y cuatro tablas de imports SMX
  verificadas; los seis harnesses runtime también compilan. El fixture de 733
  claves se regenera idéntico al usado en la aceptación humana R6.
- Inventario renovado; las dos llamadas HTTP externas de Connect con tres
  argumentos donde se requieren cinco siguen pendientes. No se modifica ese repo.
- Doce pruebas del preparador; árbol y ZIP extraído validados, integridad/checksums
  de archivos, roundtrip real entre ZIPs candidato/reversión en filesystem temporal
  con config ajena preservada y gamedata ausente retirado.
- Replay de los diagnósticos x86 extraídos, actionlint, sintaxis shell, diferencias
  Git y enlaces activos comprobados. No se ejecutó CI remota. El ensayo SRCDS
  posterior de reversión se documenta por separado.

[Logs R7](p5/r7/integration.json), [sanitizadores nativos](p5/r7/asan-native.log),
[inventario](p5/api-inventory.json), [consumidores](p5/consumers.json) y
[matriz T01–T36](p5/validation-results.json). La VM instrumentada fijada se reutiliza
como diagnóstico aislado; no forma parte del runtime instalable.

## Artefactos y reversión

Conjunto nuevo `.build/p5-r7/review/`, con manifiesto, verificación y `SHA256SUMS`:

| ZIP | Contenido |
|---|---|
| `steamworks-p5-r7-linux-x86-candidate.zip` | ELF32/include aceptados, licencia y documentación conciliada |
| `steamworks-p5-r7-competitive-dev-rollback.zip` | Extensión/include originales capturados por lectura y ausencia de gamedata registrada |
| `steamworks-p5-r7-source-review.zip` | Fuentes propias, workflow, scripts, pruebas, parches y diff contra HEAD; sin SDK/deps privados |
| `steamworks-p5-r7-vm-diagnostics.zip` | VM y receptores sintéticos x86 con licencia/procedencia; fuera de `addons` |

El payload runtime contiene únicamente extensión/include. No se agregan configs
privados, datos, tickets, módulos Steam/engine ni VM al instalable. Los cuatro ZIP
son para revisión local, sin autorización implícita de publicación o despliegue.

La [reversión R7](p5-rollback.md) renueva la captura de los mismos archivos originales.
Su roundtrip local se complementa con la [aplicación real en sitio del ZIP R7](p5-r7-rollback-srcds-acceptance.md):
hashes/ausencias restaurados, configuraciones ajenas preservadas, nuevo PID y
arranque/Steam/HTTP correctos. El baseline conserva su SIGSEGV conocido al salir;
el candidato sale limpiamente. El DEV habitual quedó restaurado.

## Pendientes vigentes

- Matriz completa de interfaces/GC, consumidores y forwards; negativos de identidad
  en host real y variantes de buffers todavía simuladas.
- Carga tardía/unload de extensión en caliente y owner unload con petición pendiente.
- Desconexión de Steam sin destruir el runtime, otras carreras de generación y
  operación sostenida/performance con medición de recursos.
- Corrección externa de las dos llamadas de Connect. Windows y otras parejas
  engine/SDK quedan fuera de esta entrega exclusivamente Linux x86.

## Reproducir sin desplegar

Seleccionar dependencias fijadas, `-m32`, `SOURCEMOD_DIR`, `MMSOURCE_DIR`, `AMTL_DIR`,
`SPCOMP` y la VM x86 instrumentada. Las suites VM usan Clang 14/ASan compartido y
`-fsized-deallocation -fno-pie -no-pie`, sin supresiones.

```sh
python3 scripts/test-integration.py --with-auth-binding --with-auth-forward \
  --with-sourcepawn-environment --output .build/mi-integracion-x86
python3 scripts/stage-artifact.py . .build/p5-r7/build/package .build/mi-candidato
EXTS_ARTIFACT_DIR="$PWD/.build/mi-candidato" bash scripts/ci-validate-artifact.sh
python3 scripts/prepare-review-artifacts.py --candidate .build/mi-candidato \
  --baseline .build/p5-r7/baseline --output .build/mi-entrega --label mi-checkpoint
```

Las salidas deben ser nuevas. Las recetas complementarias acompañan al ZIP de
fuentes; no arrancan un servidor ni instalan la VM diagnóstica.
