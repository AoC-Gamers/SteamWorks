# P4: fichas EB01–EB04 del perfil Linux x86

Responsable del análisis: Codex. Fecha: 2026-11-14. Extensión sobre develop
`46e3ee05fc29ad090a1d6f7b1cb5e3dca9855350` con revisión local P1–P4. SDK objetivo
1.58a, commit `a7aaa2754d9cda244d0cd9d7523c1144a1c472d2`, sin migración.
Validación local E3; aprobación/release de producción: ninguna.

## Identificación del entorno y módulos

Origen: instalación existente en `competitive_dev`, seleccionada expresamente
por el usuario. Juego 2.2.4.3, app build 23990100, exe build 10097. Linux x86;
SourceMod 1.12.0.7230 y Metamod 1.12.0-dev+1194. Build de extensión: GCC 12.2.0,
headers SM/MM/HL2SDK fijados en P0 y SDK 1.58a. El binario enlazado se ejecutó en
un SourceMod separado, no sobre la extensión instalada.

| Módulo real | SHA-256 | GNU build-id |
| --- | --- | --- |
| [engine_srv.so](competitive-dev/engine.json) | `8f8093a18bba14cb73fc11589687bfb8156b00328b7413d697f1a76ed4b7621a` | `b152439170af081a0d31fc357364a4cd4caa56ac` |
| [libsteam_api.so](competitive-dev/steam-runtime.json) | `43fc556d8255dd17f15d7ce266c7c7bee3075499fdec350af5ed32b9b7cda0ed` | `0d18f257d51538a7b34c594a31e042b04001fb1c` |

Los JSON incluyen tamaño/formato, origen, path del contenedor, device/inode,
exports y relocations. Se compararon hashes de copias de análisis con los archivos
mapeados del proceso. La [captura de módulos](competitive-dev/loaded-modules.json)
es histórica: el proceso de prueba se detuvo al terminar. No se publica el mapa
de direcciones virtuales ni contenido de cmdlines, entornos o autenticación.

Las cuatro [referencias SDK](modules/) siguen siendo referencias, separadas del
runtime real. La biblioteca del juego no se etiqueta como SDK 1.55 por semejanza.
La decompilación pública tampoco se considera el engine identificado.

## EB01: SteamInternal_GameServer_Init

**Necesidad/consumidor:** observación de inicio exitoso para reactivar recursos
P3, en `SteamWorksGSDetours`.

**Contrato:** función libre cdecl Linux i386, bool en AL; seis argumentos por
valor: `uint32`, tres `uint16`, `EServerMode` de 32 bits y `const char*`. Fuente
contractual de referencia: SDK 1.55 `public/steam/steam_gameserver.h`, declaración
de `SteamInternal_GameServer_Init`; la compilación permanece en 1.58a. El código
comprueba tamaños de los tipos y conserva las seis posiciones y el resultado.
No hay `this` implícito ni cambio al contrato `_V2`.

**Binario/call site:** función local `CSteam3Server::Activate`, RVA `0x201720`,
prepara los seis slots y comprueba AL tras la llamada. Relocation `R_386_PC32`
en `0x2018ec` contra ese export. El proveedor mapeado lo exporta en `0x1b430`.
El wrapper `InitSafe` es otra función; ya no se usa como observador.

**Resolución:** export exacto del proveedor con perfil de build-id, dirección
`base + RVA` y segmento ejecutable. Prefijo original de nueve bytes: cuatro
pushes y call PIC. Es una comprobación de entrada acreditada, no un patrón de
búsqueda. La [ficha de perfil](competitive-dev-linux-x86.json) y el selector C++
se comprueban mutuamente para evitar duplicaciones editables divergentes.

**Trampoline/ciclo de vida:** CDetour real ejecutó el original y devolvió éxito
al engine en cuatro inicializaciones del primer proceso, más otro arranque
frío. Los ciclos posteriores a shutdown conservaron el proceso y reactivaron
los recursos. El módulo abierto se mantiene hasta destruir trampolines. Un fallo
de creación revierte el par; no se habilita solo uno.

**Negativas:** identidad desconocida/ambigua/arquitectura ajena, export ausente,
dirección/prefijo no acreditado y fallo parcial rechazan antes de parchear.
El test de política real no abre bibliotecas en un proceso sin esta pareja.

**Decisión:** validada localmente para el perfil exacto, E3. No constituye aprobación
de otros proveedores, de Windows ni de un prototipo moderno de inicialización.
Revisor/release de producción: null; no se simula aprobación humana.

## EB02: SteamGameServer_Shutdown

**Necesidad/consumidor:** retirar hooks/callbacks/HTTP y resetear interfaces antes
del original. Contrato público SDK 1.58a: void, sin argumentos, `S_CALLTYPE`.

**Binario:** `CSteam3Server::Shutdown`, RVA `0x200db0`, llama mediante relocation
`0x200dca`; export del proveedor en `0x1b690`. Prefijo original de ocho bytes:
tres pushes y call PIC. La dirección se contrasta con la misma pareja y región
que EB01. No se admiten firmas sustitutivas de gamedata.

**Pruebas:** cuatro shutdowns registrados en el primer proceso con el CDetour
real; los reinicios posteriores prueban retorno/continuación del motor. La
retirada y recarga del módulo/harness volvió a instalar el par, por lo que los
prefijos fueron restaurados antes de la segunda instalación. Un segundo proceso
repitió arranque/ciclo con las configs de laboratorio completas.

**Propiedad:** observador owned por `SteamWorksGSDetours`; interfaces prestadas
al runtime. El coordinador P3 desregistra consumidores antes del original y no
recrea interfaces para desmontar. Destrucción de detours antes de cerrar la
referencia propia al proveedor. No se cambia SDK ni se sustituye el archivo Steam.

**Decisión:** validación local E3, mismo alcance de EB01. No se certifica toda
secuencia de descarga posible ni una descarga del propio código en su pila activa.

## EB03: interfaces y factory

`SteamInternal_CreateInterface(const char*)` tiene contrato declarado en el SDK
objetivo. Es el único factory alternativo utilizado; se eliminan el factory
interno no acreditado y sus reemplazos por firmas. La ruta global SteamClient
preexistente mantiene prioridad. El ensayo de estado confirma disponibilidad,
conexión e IP; no distingue por sí solo qué ruta devolvió el objeto ni certifica
todos los métodos de cada interfaz. Las versiones/tipos siguen en el registro P2.

Los getters de contexto y el global SteamClient aparecen en el proveedor real.
Las identidades de las dos copias de steamclient mapeadas quedan registradas como
contexto, sin afirmar equivalencia entre ellas ni reconstruir sus vtables. No se
crea un perfil privado de interfaces a partir de esa observación.

**Decisión:** uso de contrato público y pruebas de estado limitado; HTTP/GC/stats
completos y demás runtimes siguen requiriendo aceptación propia.

## EB04: helpers, configuración y paquete

`SteamWorksMemUtils`, `ResolveSymbolInt` y `GetOffsetFromVTable` no tenían
consumidores fuera de sus archivos ni entradas en AMBuilder; fueron retirados.
Se elimina esa ruta de `strlen` sobre patrones y búsqueda no acotada de vtable.
No se añade un scanner ni índice de reemplazo. T13/T14 corresponden a retirada.

El perfil usa exports, no gamedata nuevo. Los manifiestos son documentación en
`docs/`; el validador rechaza activación JSON, inconsistencias, autoaprobación,
patrones e índices inventados. T26 pasó con el paquete enlazado real, sin copiar
SDK/plantillas activas. No hubo ejecución remota de CI ni publicación.

## Evidencia y reversión de la prueba

[Traza seleccionada](competitive-dev/lifecycle-trace.json),
[cleanup](competitive-dev/cleanup.json) y
[resultados finales](validation-results.json). El laboratorio y su CFG se
retiraron; la extensión instalada conserva su hash y el servidor quedó detenido
como al inicio. Las copias de binarios, disassembly y logs técnicos de build
permanecen solo en `.build/p4/competitive-dev/`. No se redistribuyen esos binarios.
