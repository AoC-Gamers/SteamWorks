# Cobertura y límites

Esta matriz describe comportamientos y procedencia. No es una colección de logs
ni una ejecución nueva; los resultados actuales se guardan fuera de Git.

Las pruebas locales de GC cubren ABI, buffers, forwards y ciclo de vida con
proveedores controlados. La API se conserva por compatibilidad; su funcionamiento
depende del soporte del juego y del contexto Steam. Consulta las
[dependencias funcionales](../docs/game-coordinator/README.md#aplicabilidad-a-l4d2-dedicado-x86).

La recepción real de `SteamWorks_OnUserStatsUnloaded` queda **excluida de la
aceptación global de L4D2 dedicado x86**: no se ha identificado un disparador
reproducible y Valve no garantiza una descarga al desconectar ni un plazo de
emisión. No se considera aprobada por esta exclusión. Se conservan la API y las
pruebas deterministas de entrega, aislamiento cliente/GameServer y teardown,
incluidos sanitizadores y controles negativos. Consulta el
[límite funcional](../docs/stats/README.md#descarga-de-caché-en-l4d2-dedicado-x86).

| Contrato | Suite/escenario | Nivel | Evidencia runtime histórica | Límite |
|---|---|---|---|---|
| Carga propietaria por DSO | extension-entrypoint; negativo sin guarda del factory | Wrappers de producción y cargador/host controlados, ASan/UBSan x86 | 2026-11-17; Linux x86; ambos órdenes rechazaron el duplicado preservando HTTP pendiente, consumidor fallido recuperado, recarga en el mismo proceso y cierre limpio | Payload `a7764ee0181e530fee83040d82ac9087e2863c751df6a21c0c8b1599493d2398`; no acredita stats ni runtime Windows |
| API y firmas/opcionalidad | Inventario original de API + adiciones explícitas + integración | Inventario local | No necesaria para comparar firmas | No prueba ABI/runtime |
| Índice de cliente independiente de entidad | client_index_test en stats-natives y steamid-string | Natives de producción y dobles locales x86 | No necesaria para la regresión local | Rango/conexión e identidad existente; no acredita una conexión real |
| Stats: notificaciones asíncronas y descarga de caché | stats-callbacks; contrato de adiciones API | SDK real + proveedor y despacho por contexto controlados, ASan/UBSan x86; negativo cliente/GameServer | 2026-11-17; Linux x86; Received con identidad/AppID correctos, lecturas y recarga soportada en el mismo proceso | Payload `d3814a2d69ab69108246b05701d5fee14d38ad8220274b1c448b59ffcaf4744c`; recepción real de Unloaded no acreditada y excluida del alcance global; silencio de callbacks limitado a la ventana observada |
| Observador de caché y entrega de stats | steamworks_stats_callbacks; estados watching/matched_unloaded | Harness SourcePawn y SRCDS | 2026-11-17; Linux x86; dos Received correctos y lecturas válidas; objetivo retenido tras finalizar/desconectar durante cinco minutos sin nuevas solicitudes | Payload `a7764ee0181e530fee83040d82ac9087e2863c751df6a21c0c8b1599493d2398`; cero Unloaded, recepción real no acreditada y excluida del alcance global, sin garantía de plazo |
| Stats: fallo conserva salida, identidad y AppID | stats-natives; harness stats | Dobles y SRCDS | 2026-11-15; Linux x86; F2; stats enteros/float y claves inexistentes (R3/R4), usuario autorizado | Solicitud aceptada no significa lectura disponible |
| Identidad: capacidad/NUL y preautorización | steamid-string; harness identity/consumer | Dobles y SRCDS | 2026-11-15/16; Linux x86; F4; buffers (T15), consumidores (T25) y preautorización (T29) | No inferir grupo/perfil desde identidad |
| Auth 014, ticket y agregación | auth-binding/auth-forward | SourceHook/VM reales offline y SRCDS | 2026-11-15; Linux x86; F1/F2; adaptador y autenticación | Proveedor offline no emite ticket real |
| Init/teardown con SourceTVSupport | detours/init-coexistence/resource-lifecycle | Dobles/x86 y SRCDS | 2026-11-15; Linux x86; F3; cierre y reinicialización (T08) en mismo proceso | No toda la pila de consumidores |
| Streaming: dos celdas y payload | http-stream-context | Wire independiente y SRCDS | 2026-11-15; Linux x86; F2; cuatro tandas, una tras mapa | No todo endpoint remoto |
| HTTP estricto y GC snapshot | typed-boundary/resource-lifecycle, negativos mínimos | Proveedores controlados locales | No necesaria para las regresiones locales | Contratos de buffers y forwards; no valida el protocolo del backend |
| Cero/uno/varios forwards | auth-forward; harness forward | VM real y SRCDS | 2026-11-15; Linux x86; F4; agregación de forwards (T23) | ET_Event no propaga -1 individual como agregado |
| Capacidades e interfaces | binary-evidence/typed-boundary; harness capability | Metadata/registro y SRCDS | 2026-11-15; Linux x86; F4; matriz de interfaces/capacidades (T11), GC opcional | Exports no son aceptación Windows |
| Propietario HTTP pendiente y carga tardía | resource-lifecycle; harness pending/lifecycle | Dobles y SRCDS | 2026-11-15; Linux x86; F4; descarga del propietario HTTP (T20) y carga/recarga de extensión (T16) | Descarga forzada con dependientes no soportada |
| Instalación y reversión real | review_artifacts; harness rollback | Filesystem y SRCDS | 2026-11-16; Linux x86; F5; instalación (T26) y reversión (T35), revisión R19, en árbol detenido | Runtime mínimo, no conjunto completo |
| Destrucción VM | sourcepawn-environment | VM real ASan offline | No se instala en servidor | No acredita VM distribuida |
| Paquete limpio y rutas documentales | Python/documentación y staging/CI | Offline | No requiere SRCDS | Windows runtime pendiente |

## Atribución histórica

Las etiquetas entre paréntesis son referencias al plan interno de pruebas, no
versiones publicadas ni resultados por sí mismas. `T` identifica un escenario
(la matriz indica su comportamiento); `R` identifica una revisión de la entrega
de integración. Por ejemplo, R3/R4 agrupan los ensayos históricos de stats, R19
el ensayo de instalación/reversión citado y R20 la validación posterior offline.
Su aceptación se limita a las fechas, hashes, observaciones y consumidores
indicados aquí. Un número mayor no transfiere aceptación a otro ZIP o binario.

Los identificadores F1–F5 distinguen los payloads de ensayos históricos; no son
nombres de entornos ni niveles de soporte. Los hashes, fechas y límites de esta
matriz conservan la atribución sin publicar detalles de instalaciones locales.
Las capturas originales no equivalen a una nueva ejecución ni se transfieren
entre binarios por compartir número de revisión documental.

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
Ensayos anteriores sobre `839ff075…` mantienen su propia atribución en esta matriz.
Las 15 suites y build de R20 no equivalen a reinstalar su ZIP nuevo. El quit
limpio del perfil mínimo no declara resuelto el SIGSEGV histórico del baseline.
Runtime Windows permanece pendiente.

## Migración a SDK 1.55

El payload SDK 1.55 conserva la API del inventario original y tiene SHA256
`abb4d03a47f88315950ba8a63c2a1696d8daa80cf52039c808e95569f6adad52`;
el include público conserva el SHA256 indicado arriba. La aceptación siguiente
es una observación propia del 2026-11-16 en Linux SRCDS x86, independiente de F1–F5.

| Escenario SDK 1.55 | Observación | Límite |
|---|---|---|
| Init y auth 014 | Interfaz nativa y prefijo auth resueltos, conexión Steam; BeginAuthSession y ambos forwards de validación con cuenta autorizada | No todos los resultados de autenticación |
| Identidad | 40 checks sin fallos; límites/NUL/canarios y rechazo de preautorización | No revalida grupo, perfil ni préstamo |
| Stats | Enteros por cliente/AccountID y 12 claves float coinciden; 733 claves examinadas; claves inexistentes conservan salidas | No todos los stats de todas las cuentas |
| HTTP | Ocho casos antes y después del mapa, 16 completados sin fallos; streaming con contexto, offsets y payload correctos | Fixture controlado, no todo endpoint remoto |
| Reinicialización | Mismo proceso, runtime_stop/runtime_start y reconexión; HTTP pendiente cancelado sin callback tardío | Cinco harnesses y extensión SourceTVSupport, no toda la pila |
| Cierre y restauración inicial | Quit limpio; selector, configuración, plugins y extensiones originales preservados | Alcance del ensayo inicial |
| Instalación desde ZIP (T26) | ZIP candidato identificado por hash, Steam/HTTP y quit limpio | No instala automáticamente ZIP regenerados |
| Reversión corregida (T35) | Baseline nuevo cualificado antes del candidato; restauración de bytes/ausencias y Steam/HTTP; tres quits limpios, sin coredumps asociados | Un harness y SourceTVSupport; no downgrade funcional ni toda la pila |

Los negativos generaron 15 excepciones nativas esperadas, conciliadas con sus
casos y sin errores inesperados; las comprobaciones y logs permanecieron activos.
SourceTVSupport estaba cargado, pero su VSP no cargó tanto en candidato como en
baseline: no se acepta emisión/VSP. El cierre limpio mínimo no declara resuelto
el SIGSEGV histórico de otros consumidores. La selección exacta del SDK, binding,
marshalling y negativos se cubren además offline. Runtime Windows permanece pendiente.

## Baseline de reversión con finalizador de la sesión cliente Steam

Observación del 2026-11-16 en Linux SRCDS x86, con fuentes de SDK 1.55 y API intacta:

| Archivo/payload ensayado | SHA256 |
|---|---|
| ZIP candidato del primer ensayo de instalación/reversión | `65d63b8010376381e44434d1405e2c0a8b9298632ce9a7678e1251cae9fe2442` |
| ZIP candidato del segundo ensayo de instalación/reversión | `262939e0f13d8e63d7b37b4267ce82ae595b97eb6876661ce7d9e7fee2ae56d2` |
| ZIP candidato del tercer ensayo de instalación/reversión | `34b9ea85013bd12c303e1a6d523ef76a870c93c502f2ea98a31c347766fd136e` |
| ZIP candidato del cuarto ensayo de instalación/reversión | `0df22354c93fa640b5e29300a402f51f15687b6ecfe6c1198aa6da8f629ae682` |
| ZIP candidato del quinto ensayo de instalación/reversión | `13622bc535b30dbcfdf4d6e1afe1e8a2cce50562d75301990d16639380cb63fe` |
| ZIP candidato del sexto ensayo de instalación/reversión | `b3f81f3466717444f05f0f65c885d643a2924125adc23850549ad2ab2ccb109d` |
| ELF baseline nuevo, sin optimización | `678e334bcfae091461cf43b10ed6d487780fc34d354b9ad7c4b0826747e71bbd` |
| ZIP rollback nuevo | `3231b4a368ef1ae0552a0f6795de1b69aece82def8e96f63b625ff12e785a7ee` |
| ELF baseline histórico fallido | `9ca37aadcd83448d49126d6a72babcb8ec97ec5a06c171b4821ae0ee3cf05d70` |
| ZIP rollback histórico preservado | `55153d557f3af7f71f1016c8c394dc74fe29e0cdc9511044c21de29e8bc6dffc` |

El baseline nuevo usa el mismo snapshot auditado que el candidato y conserva
el finalizador de la sesión cliente Steam; no reconstruye el baseline viejo ni revierte a
funcionalidad anterior. Se cualificó por arranque/Steam/HTTP/quit, se instaló el
candidato y se restauró el ZIP nuevo en el mismo árbol aislado. Contexto, cuerpo,
callback, cierre HTTP, archivos/ausencias propios y configuración/plugin ajenos
pasan. Los tres procesos salieron limpiamente y no registraron coredumps asociados.

El SIGSEGV del baseline histórico tiene diagnóstico propio de acceso a interfaz
de crashhandler tras descargar el módulo, seguido de fallo del manejador de señal.
No se suprimieron comprobaciones ni se alteró ese archivo para aceptar la reversión.
En el primer ensayo no se midió la rama condicional del pipe cliente. En los
ensayos posteriores identificados en la tabla, los tres desmontajes de cada
ensayo registraron
`client_host_shutdown active_before=1 active_after=0 server_active=0`: el cliente
estaba activo y se cerró después de detener GameServer. Los tres procesos
salieron limpiamente, sin coredumps asociados, y se preservaron los archivos
originales. La observación nueva no cambia los límites del ensayo anterior;
las guardas conservan cobertura offline. La evidencia del fallo histórico
permanece válida y separada. Un ZIP documental regenerado requiere su propia
instalación/reversión en sitio; los bytes de runtime idénticos mantienen solo
su atribución binaria. Runtime Windows sigue pendiente.
