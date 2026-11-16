# P4: perfil binario Linux x86 y ruta real de lifecycle

Fecha: 2026-11-14. Rama `develop`, base
`46e3ee05fc29ad090a1d6f7b1cb5e3dca9855350`. Se conservan P0/P1/P2/P3 y toda la
revisión local anterior. Sin staging Git, commit, publicación ni sustitución de
la extensión instalada. API pública SourcePawn conservada.

## Resultado

Se identificó y validó localmente un perfil mínimo para la instalación del
contenedor `competitive_dev`, seleccionado por el usuario:

| Campo | Observado |
| --- | --- |
| Juego | Left 4 Dead 2, patch 2.2.4.3, app 222860 build 23990100 |
| Engine | Exe build 10097, 2026-06-30; ELF32 i386 |
| SourceMod | 1.12.0.7230 |
| Metamod | 1.12.0-dev+1194 |
| Perfil | `l4d2-23990100-linux-x86` |
| SDK de compilación | 1.58a, sin migración |

El [manifiesto del perfil](p4/competitive-dev-linux-x86.json) enlaza los módulos,
relocations, prototipos y pruebas. Las [fichas de evidencia](p4/binary-evidence.md)
describen contrato, resolución, propiedad y límites.

La instalación estaba presente, pero no había SRCDS activo al inspeccionarla.
Se arrancó únicamente esa instancia para identificar módulos y versiones; se
comprobó que no había humanos conectados antes de detenerla. La validación de
la extensión nueva se hizo después con un árbol SourceMod `sourcemod_p4`
separado, solo el harness del proyecto y escucha de juego en loopback. Se
conservaron la extensión instalada y las bibliotecas Steam. El laboratorio se
retiró al terminar y SRCDS quedó detenido, como al inicio; el contenedor y sus
sesiones administrativas siguen activos.

## Ruta real y ABI

El engine identificado contiene llamadas con relocations `R_386_PC32`:

| Función del engine | Relocation | Export del proveedor |
| --- | --- | --- |
| `CSteam3Server::Activate` | `0x2018ec` | `SteamInternal_GameServer_Init` |
| `CSteam3Server::Shutdown` | `0x200dca` | `SteamGameServer_Shutdown` |

Los operands de `Activate` preparan seis slots de pila y el retorno se comprueba
en AL. El prototipo legacy coincide con el contrato de referencia declarado en
el SDK 1.55: bool, IP de 32 bits, tres puertos de 16 bits, `EServerMode` de 32 bits
y puntero a versión. Se conserva esa ABI al compilar con SDK 1.58a. No se atribuye
una versión SDK al archivo del juego a partir de una semejanza de instrucciones.

El fork interceptaba `SteamGameServer_InitSafe`. En las referencias examinadas
es un wrapper que llama a otra entrada, y no prueba que el engine pase por él.
Ahora se intercepta el export real `SteamInternal_GameServer_Init`, conservando
argumentos y resultado. Shutdown conserva su contrato público void sin argumentos.
No se llama a una inicialización artificial para fabricar evidencia.

La [decompilación pública de Activate](https://binaries.l4d2node.org/b/engine_srv.so/f/0x00211680)
y [Shutdown](https://binaries.l4d2node.org/b/engine_srv.so/f/0x00210d10) orientó la
investigación. La aprobación local utiliza los bytes y relocations de los módulos
identificados; los offsets de esa decompilación no se reutilizan como perfil.

## Selección y resolución

`core/swlifecycleprofile.h` permite únicamente Linux i386 y exige:

1. Un único `engine_srv.so` y un único `libsteam_api.so` en el proceso.
2. GNU build-id de ambos módulos igual al registrado.
3. Export del proveedor en la RVA medida, dentro de un `PT_LOAD` ejecutable.
4. Prefijo original exacto de cada entrada; una entrada ya parcheada se rechaza.

La lectura de notas GNU usa longitudes explícitas y límites de segmentos. El
selector no hace búsqueda por patrones ni recorre vtables. SHA-256 identifica
los archivos en la captura documental; el selector de runtime compara build-id,
unicidad, RVA, segmento y prefijo, sin hashear archivos durante el juego.

Los exports se resuelven antes de crear ambos detours. Ya no se consultan firmas
de gamedata para reemplazar estas entradas. Se mantiene una referencia propia
a la biblioteca hasta destruir los dos trampolines. El factory alternativo de
SteamClient usa exclusivamente `SteamInternal_CreateInterface` con la convención
de llamada declarada en el SDK; se retiró el fallback interno no acreditado.

El perfil se compila en el selector. Los JSON son documentación y no lo activan;
su campo `enabled=false` expresa esa separación. Cambiar el manifiesto o añadir
gamedata no habilita otro binario. El validador comprueba coherencia de build-id,
RVA/prefijo de código, metadata de archivo y traza. No hay perfil Windows ni se
aceptan otras arquitecturas o parejas por similitud.

## Pruebas reales

La [traza seleccionada](p4/competitive-dev/lifecycle-trace.json) registra:

- Cuatro inicializaciones exitosas y cuatro shutdowns en el primer proceso de
  laboratorio, con continuación del motor tras ejecutar los originales.
- Dos instalaciones del par: carga inicial y descarga/recarga de extensión y
  harness. La segunda instalación exige que se hayan restaurado los prefijos.
- Un segundo arranque frío, corregidas las omisiones de configuración inicial
  del laboratorio, y otro ciclo observado sin errores propios de SteamWorks.
- Natives de estado con `loaded=1`, `connected=1`, `vac=0` e IP consultable.
  VAC desactivado corresponde al laboratorio `-insecure`; se omite la IP del informe.

Esto ejercita el CDetour real y el motor real, incluyendo relocación PIC,
trampolines y retorno del original. El nuevo binario se compiló y enlazó completo
con GCC 12/multilib y `-Werror`; se eliminó el flag C que impedía compilar C++.
El rebuild final produjo el mismo SHA-256 que el artefacto cargado en el laboratorio.
El harness se compiló con SourcePawn 1.12.0.7259 sin warnings.

La instalación inicial del laboratorio omitía configs públicas de lenguajes y
administración; se completaron y se repitió el arranque frío. Esos errores de
fixture no se presentan como fallos de la extensión ni se ocultan como una
primera ejecución limpia.

## Pruebas deterministas y paquete

```bash
bash scripts/test-binary-evidence.sh
TEST_CXXFLAGS=-m32 RUNTIME_MODULE_PATH=/ruta/al/modulo/identificado/libsteam_api.so \
  bash scripts/test-binary-evidence.sh
bash scripts/test-resource-lifecycle.sh
```

Pasaron 17 pruebas Python con subcasos negativos, los casos de relocación de
prefijos SDK y del proveedor real, la política de rechazo sin módulos acreditados,
18 checks de identidad en x86_64 y 25 en x86 (incluidas notas GNU truncadas y
rangos inválidos), y todas las regresiones P1/P2/P3 en ambas arquitecturas.
La suite de lifecycle también pasó con ASan/UBSan y detección de fugas en x86_64.
Los prefijos del decoder x86 se comprueban con su compilación normal; no se
atribuyen al decoder resultados de sanitizer que no se ejecutaron sobre él.

Los helpers de memoria sin consumidores quedaron retirados. T13/T14 corresponden
a esa retirada; no se añade otro scanner ni un índice de vtable supuesto. Se
creó y validó el paquete con el binario enlazado real. Los perfiles quedan en
`docs/`; no se instala un `steamworks.txt` ni se redistribuyen binarios/headers SDK.
La API y el escaneo de consumidores siguen equivalentes a P0. CI se validó con
actionlint localmente; no hubo ejecución remota.

## Alcance de aceptación

T12 tiene evidencia estática y ejecución controlada para esta pareja Linux x86.
T26 tiene build/paquete local real y ensayo aislado del artefacto. T31 tiene
rechazo de módulos desconocidos/ambiguos, arquitectura ajena, dirección no
acreditada y prefijo distinto antes de crear parches. Los resultados son E3 de
validación local; no equivalen a despliegue de producción ni a E4.

No se certifican otros binarios, Windows, todas las operaciones HTTP/GC/stats,
clientes reales ni operación prolongada. Se conserva el límite de lifecycle
serial de SourceMod/Steam y no se valida descargar el módulo binario mientras
uno de sus frames C++ esté activo. La API `EnableDetour` sigue siendo void; la
traza demuestra habilitación efectiva en este entorno, sin generalizar políticas
de protección de memoria de otros hosts.

Evidencia y hashes: [validation-results.json](p4/validation-results.json).
La [captura inicial sin runtime](p4/initial-no-runtime-validation-results.json)
y el candidato anterior se conservan como antecedentes. La selección del usuario
y los módulos de `competitive_dev` resolvieron esa dependencia.

## Seguimiento del punto de init

La ruta de export descrita arriba corresponde a P4. La [corrección posterior de
convivencia](t08-init-coexistence.md) observa el `CALL` ya acreditado del motor y
conserva el export para SourceTVSupport. Su admisión y regresiones x86 pasan;
la aceptación real del nuevo ELF sigue pendiente. Los resultados históricos P4
y su manifiesto no se reescriben.
