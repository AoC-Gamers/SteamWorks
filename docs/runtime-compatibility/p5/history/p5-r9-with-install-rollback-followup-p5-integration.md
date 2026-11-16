# P5-R9: integración del build optimizado aceptado en SRCDS x86

Rama `develop`, base `46e3ee05fc29ad090a1d6f7b1cb5e3dca9855350`.
El build completo con `--enable-optimize -O3 -DNDEBUG -m32 -Werror` reproduce
`839ff0751b04141031bdc6dffc48662d0804f52227f2c91015bd87c6b84cf192`, el ELF
[aceptado directamente en SRCDS](optimized-srcds-acceptance.md). La API pública
se conserva. **P5 no tiene aceptación global.** La preparación ejecutó builds y
pruebas locales y capturó la reversión por lectura. El seguimiento autorizado
[T26/T35](p5-r9-rollback-srcds-acceptance.md) instaló los ZIP reales en SRCDS aislado
y verificó restauración/arranque/HTTP. Sin publicación ni producción.

## Evidencia conciliada por hash

| Evidencia del ELF optimizado `839ff075…` | Resultado |
|---|---|
| Lifecycle con SourceTVSupport | Arranque frío y dos reinicializaciones completas en el mismo PID 87189; un forward de conexión por reinit; CALL del engine/shutdown en SteamWorks y bridge init del SDK en SourceTVSupport |
| HTTP/streaming | 25 completions sin fallos, 18 bytes y tres eventos headers con ambas celdas del contexto; texto/binario/vacío/503/timeout/TLS inválido/conexión rechazada |
| Cancelación y generación | Respuesta retenida a través de shutdown, liberada tras pasar HTTP en el runtime nuevo antes de 60 s; siete segundos sin callback obsoleto; cierre seguro del handle y cancelación inmediata sin callback |
| Autenticación e identidad | Un humano autorizado, un BeginAuthSession 014, despacho SourceMod 0 y validación Steam 0 separados; un evento en cada forward público; identidad/buffers/licencia de AppID 550 correctos |
| Stats de solo lectura | Entero coincide por cliente/AccountID; lectura de clave entera como float falla preservando salidas. Dos recorridos de 733 claves, las mismas 12 floats finitas coincidentes; 16 lecturas de clave inexistente preservan todas las salidas, incluidos bits de NaN |
| Quit y restauración | Tres shutdown contando quit; sin SIGSEGV, errores SourceMod ni nuevo core del candidato. DEV habitual conectado a Steam, cuatro hashes protegidos restaurados y laboratorio retirado |

[Resultados runtime](optimized-srcds/results.json) y [restauración final](optimized-srcds/cleanup.json).
Los ensayos usaron LinuxGSM, copia SourceMod aislada en 27015, flags/configs
habituales y ninguna contraseña añadida. No se inspeccionaron/exportaron tickets,
AccountIDs del forward ni valores personales de stats; no se escribieron stats.

La [corrección del build optimizado](optimized-build.md) valida la reserva acolchada
del ticket contra `PTRDIFF_MAX` y la capacidad del respaldo fijo de CDetour.
La compilación optimizada ahora pasa sin suprimir comprobaciones. La aceptación
anterior de `82347b99…` permanece histórica. El build canónico `75a9b5d9…` no se
prueba en SRCDS ni se empaqueta aquí. El primer ensayo optimizado copió el
ELF/include local; el seguimiento T26 instaló después el payload desde el ZIP R9
real dentro del contenedor y T35 aplicó el rollback en el mismo árbol detenido.

## Validación integrada repetida

- Build optimizado x86 completo con warnings como errores; SHA idéntico al módulo
  realmente mapeado en SRCDS. Los 55 headers/dependencias del SDK coinciden con P0.
- **14 suites x86 pasan**, incluida comparación de declaraciones, registros, valores,
  typesets y opcionalidad de la API pública con P0.
- 88 comprobaciones de hooks/lifecycle, con límites de reserva del ticket; 318 del
  respaldo real de CDetour y 27 escenarios del owner/wrapper. Los casos relevantes
  se compilan con `-O3 -DNDEBUG` y checks que siguen activos.
- 142 comprobaciones de convivencia ejecutando código x86, 21 de admisión del
  llamador y control negativo; 18 de binding 014 con SourceHook y control negativo 015.
- 227 comprobaciones CForward/VM, 16 ciclos de propiedad de la VM y 92 del contexto
  streaming con control negativo. ASan/LSan/UBSan sin supresiones, incluidos lifecycle,
  respaldo CDetour, streaming y convivencia nativos.
- Tres consumidores recompilados sin warnings, cuatro tablas de imports SMX y seis
  harnesses runtime compilados. Fixture de 733 claves idéntica a la aceptación optimizada.
- Inventario renovado: 2688 archivos, 34 con referencias. Persisten dos llamadas
  HTTP externas de Connect con tres argumentos donde se requieren cinco; ese repo
  no se modifica ni se declara su aceptación runtime.
- Doce pruebas del preparador, validación de árbol/ZIP extraído, integridad y hashes;
  roundtrip de los ZIP candidato/reversión en filesystem temporal, retirando el
  gamedata declarado ausente y preservando configuración ajena.
- Replay de los diagnósticos x86 extraídos; actionlint, sintaxis shell, enlaces
  activos y `git diff --check`. No se ejecuta CI remota ni otro ensayo SRCDS.

[Logs R9](p5/r9/integration.json), [sanitizadores nativos](p5/r9/asan-native.log),
[inventario](p5/api-inventory.json), [consumidores](p5/consumers.json) y
[matriz T01–T36](p5/validation-results.json). La VM instrumentada fijada se reutiliza
como diagnóstico aislado; no forma parte del runtime instalable.

## Artefactos y reversión

Conjunto nuevo `.build/p5-r9/review/`, con manifiesto, verificación y `SHA256SUMS`:

| ZIP | Contenido |
|---|---|
| `steamworks-p5-r9-linux-x86-candidate.zip` | ELF32 optimizado/include, licencia y documentación conciliada |
| `steamworks-p5-r9-competitive-dev-rollback.zip` | Extensión/include originales capturados por lectura y ausencia de gamedata registrada |
| `steamworks-p5-r9-source-review.zip` | Fuentes propias, workflow, scripts, pruebas, parches, recetas y diff contra HEAD; sin SDK/deps privados |
| `steamworks-p5-r9-vm-diagnostics.zip` | VM y receptores sintéticos x86 con licencia/procedencia; fuera de `addons` |

El payload runtime contiene solo extensión/include, sin configuraciones privadas,
datos, tickets, módulos Steam/engine ni VM. Los cuatro ZIP son para revisión local,
sin autorización implícita de publicación o despliegue. Los paquetes anteriores y
la [matriz anterior con seguimientos](p5/history/p5-r8-with-optimized-followups-validation-results.json)
permanecen intactos.

La [reversión R9](p5-rollback.md) conserva los mismos bytes originales y su ZIP
coincide byte a byte con el rollback R7 probado. La aplicación en sitio de R7
seguía al candidato antiguo `82347b99…` y conserva su atribución. El
[seguimiento R9](p5-r9-rollback-srcds-acceptance.md) sí restaura después del optimizado
`839ff075…`: archivos/configs correctos y nuevo PID con Steam/HTTP y 135 plugins
idénticos. El candidato sale limpio; el baseline conserva su SIGSEGV conocido al
salir. El DEV habitual quedó restaurado y los ZIP de revisión permanecen intactos.

T26/T35 pasan en el alcance de instalación/restauración/arranque/HTTP del seguimiento
R9. La [matriz previa](p5/history/p5-r9-before-install-rollback-acceptance-validation-results.json)
queda guardada. Esto no acepta el cierre limpio del baseline ni toda la matriz.

## Pendientes vigentes

- Matriz completa de interfaces/GC, consumidores y forwards; más negativos de
  identidad y variantes de buffers en host real.
- Carga tardía/unload de extensión en caliente y owner unload con petición pendiente.
- Desconexión de Steam sin destruir el runtime, otras carreras de generación y
  operación sostenida/performance con medición de recursos.
- Corrección externa de las dos llamadas de Connect. Windows y otras parejas
  engine/SDK quedan fuera de esta entrega exclusivamente Linux x86.

## Reproducir localmente

Usar dependencias fijadas, `-m32` y `--enable-optimize`. La receta de build y los
runners usados se incluyen en `review/` del ZIP de fuentes. Ajustar sus rutas de
salida a directorios nuevos. Las suites VM usan Clang 14/ASan compartido y
`-fsized-deallocation -fno-pie -no-pie`, sin supresiones.

```sh
python3 scripts/test-integration.py --with-auth-binding --with-auth-forward \
  --with-sourcepawn-environment --output .build/mi-integracion-x86
python3 scripts/stage-artifact.py . .build/p5-r9/build/package .build/mi-candidato
EXTS_ARTIFACT_DIR="$PWD/.build/mi-candidato" bash scripts/ci-validate-artifact.sh
python3 scripts/prepare-review-artifacts.py --candidate .build/mi-candidato \
  --baseline .build/p5-r9/baseline --output .build/mi-entrega --label mi-checkpoint
```

Las salidas deben ser nuevas. Estas recetas no arrancan un servidor ni instalan
la VM diagnóstica.
