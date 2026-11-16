# Frontera tipada

`swtypes.h` separa AppID, AccountID, identidades individuales, referencias de
cliente y longitudes. No hay conversión implícita de AppID a AccountID. El bit
alto del AccountID se conserva aunque la celda Pawn se interprete con signo.
La identidad validada debe ser individual y coherente; los límites de memoria
se comprueban antes de publicar salidas.

`swinterfaces.h` liga cada tipo C++ a una versión literal exacta y a su header.
Un macro del SDK distinto provoca error de compilación; una versión de runtime
desconocida se rechaza. Pedir el nombre de una interfaz no demuestra su ABI.
El registro mínimo se detalla en [interfaces](../runtime/interfaces.md).

Los tipos no cambian la API de SourcePawn. Los wrappers mantienen nombres,
firmas, tipos, forwards y opcionalidad del include. La comparación de inventario
con el fixture P0 es independiente de números de línea y metadatos de fuentes.
Los tests negativos compilan un SDK inventado, mezcla AppID/AccountID y mezcla
entre el prefijo auditado y la interfaz completa del SDK: deben fallar por el diagnóstico esperado, no por un include perdido.

Implementación: `extension/core/swtypes.h`, `swinterfaces.h`, `swgameauth014.h`.
Regresiones: `tests/native/typed_boundary_test.cpp`, `interface_registry_test.cpp`
y `tests/runners/test-typed-boundary.sh` en las fuentes del proyecto.

SDK 1.55 es la única dependencia admitida: `steamworks-sdk.lock.json` fija los
headers y las dos bibliotecas x86 de enlace. La interfaz 015 permanece únicamente
como doble independiente rechazado, sin utilizar un SDK alternativo.
