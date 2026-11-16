# Aceptación del adaptador 014 en SRCDS x86

Fecha local: 2026-11-14, America/Santiago. `develop`, sin commit ni producción.
**El enlace de BeginAuthSession pasa la aceptación acotada en el perfil probado**,
con una cuenta del operador, debug 12 y cierre explícito del runtime. El candidato
aceptado es la revisión R3, SHA-256
`ddfdf9bf2f9bde625719ba171c59f7cac8b56cbe136a15e4911c7acbf00383ad`.
El candidato original `6438c716…` no cargaba; no se le atribuyen estos resultados.

## Entorno y aislamiento

Se usó `competitive_dev`, LinuxGSM `/app/l4d2server`, red host y puerto 27015.
Se detuvo temporalmente el DEV habitual y se seleccionó
`addons/sourcemod_acceptance`. El único cambio del CFG LinuxGSM fue `basepath`;
se conservaron parámetros, CFG de juego, SourceTV y contraseña habituales.

El árbol aislado copió plugins, extensiones, configuraciones y datos del DEV,
como en el reintento anterior que admitió al jugador. Solo en esa copia se
sustituyeron SteamWorks, el include y el harness; se desactivó el actualizador
SourceMod. No se cambiaron bibliotecas del juego, VM, puertos ni CFG globales
para superar un fallo. La entrega es exclusivamente ELF32 little-endian i386.

La identidad de los módulos mapeados se verificó mediante dispositivo/inodo y
hash, incluida la extensión R3, engine, libsteam_api, ambos steamclient y tier0.
El perfil sigue siendo `l4d2-23990100-linux-x86`. Se utilizó la VM instalada
SourceMod 1.12.0.7230, sin instrumentación ASan ni sustitución de SourcePawn.

## Bloqueo de carga y corrección

| Revisión | Resultado |
|---|---|
| Original, `6438c716…` | SourceMod y el probe `RTLD_NOW \| RTLD_LOCAL` rechazan el módulo: `undefined symbol: AssertValidStringPtr` |
| Primer enlace de tier0, `960740cd…` | Se incrustaba la ruta `/opt/sm-deps/.../libtier0_srv.so`; no era portable y el probe la rechazó |
| R3, `ddfdf9bf…` | Dependencia por nombre `libtier0_srv.so`; carga local y carga real en SourceMod correctas |

El código de convars recompilado desde tier1 necesita símbolos de tier0. Estar
exportados en una biblioteca del engine no los hace visibles automáticamente
en el ámbito local con que se carga la extensión. Se añadió la dependencia
explícita. La biblioteca de enlace del SDK no tiene SONAME, por lo que se usa
`-L… -ltier0_srv` para registrar el nombre portable, sin rutas del builder.
El RPATH existente y las bibliotecas originales del juego resuelven el proveedor.
No se omitieron asserts ni se añadieron stubs para encubrir el import.

El helper de build verifica ahora que `DT_NEEDED` contenga exactamente el nombre
portable. El [probe x86](../../tests/diagnostics/extension_load_probe.cpp)
reproduce la carga local sin inicializar Steam ni ejecutar interfaces de la
extensión; el caso anterior falla y R3 pasa contra los mismos proveedores reales.
El ELF conserva su arquitectura i386 y no contiene `DT_TEXTREL`.

## Entrada real y forward

El harness del laboratorio activa `steamworks_debug 12` en `OnPluginStart` y
vuelve a hacerlo si el plugin se recarga. Es una variante local del harness de
aceptación; sus fuentes y hashes se guardan con la evidencia. El receptor de
autenticación solo incrementa un contador: no inspecciona, imprime ni persiste
tickets ni identidades.

Con una conexión nueva del operador se observaron, en este orden:

```text
[hooks] event=hook_install method=BeginAuthSession success=1 version=SteamGameServer014
[auth] event=begin_auth_enter enabled=1 target=1 receivers=1 bytes=240
[auth] event=begin_auth_dispatch result=0 cells=60
[auth] event=validation_callback source=ValidateAuthTicketResponse result=0
```

El harness informó `authEvents=1`, y ambos forwards públicos de validación
contaron un evento cada uno. El forward público actual tenía otros consumidores
en el conjunto DEV; su número de receptores no se confunde con el contador del
harness. `begin_auth_dispatch result=0` es el éxito del despacho SourceMod; la
validación Steam posterior y el acceso autorizado confirman el flujo normal.

| Comprobación | Resultado observado |
|---|---|
| Steam, VAC e IPv4 | Disponibles; servidor conectado |
| Debug | Convar registrada, valor 12; snapshot con `nativeVersion=SteamGameServer015 authVersion=SteamGameServer014` |
| Enlace y transporte | Hook 014 instalado, entrada real y receptor SourcePawn invocado una vez |
| Identidad | SteamID64 coincide con SourceMod; conteos, buffer pequeño y capacidad cero pasan |
| Licencia AppID 550 | `HasLicense` (valor 0) |
| Solicitud de stats | Aceptada por cliente y AccountID |
| Stat entero | `Stat.GamesPlayed.Total` leído correctamente por ambas rutas, con el mismo valor |
| Leer esa clave como float | Falla y conserva sentinels |
| Debug apagado/encendido | Valor 0 conserva el archivo en 966 bytes; valor 12 añade snapshot y llega a 1218 bytes |

No se comparó el contenido de un ticket real ni el AccountID del argumento del
forward con la identidad del cliente. Esos detalles de marshalling mantienen su
cobertura determinista previa; aquí se verifica recepción y flujo real. No se
escribieron valores de stats ni se modificó la decisión de autenticación.

## Cierre explícito y restauración

La parada inicial de LinuxGSM por Ctrl+C terminó el proceso sin callbacks de
cierre observables; no se contó como prueba de desmontaje. Se hizo un segundo
arranque breve sin jugador y se envió `quit` desde la consola. Esta vez se vieron:

```text
[hooks] event=gameserver_detach generation=1
Steam lifecycle shutdown observed: consumers stopped
[SW-ACCEPT] plugin_end completed=0 failed=0
```

El segundo arranque tiene contadores propios, iniciados en cero. No reemplaza
ni borra la evidencia de la conexión del primero. LinuxGSM se detuvo para impedir
que su wrapper volviera a lanzar el laboratorio. El log de esa salida no contiene
marcadores de crash. No se ensayó una descarga/recarga en caliente en este ciclo.

Se restauró byte a byte el CFG original (`ca1b5179…`) y se arrancó el DEV habitual
con LinuxGSM. Se verificaron `addons/sourcemod1`, la extensión original mapeada
(`9ca37aad…`), conexión a Steam y hashes de las configs de AoC y Family Share.
Se retiró el árbol aislado completo, incluidas copias privadas, rollback local
y probe. No se publican configuraciones, credenciales, datos SQLite o logs crudos.

## Evidencia y alcance

[Resultados y hashes](auth014-srcds/results.json), trazas filtradas, módulos
mapeados, control del debug y restauración están en `auth014-srcds/`. El build
R3 está en `.build/auth014-srcds/build-r2/package/` (directorio de trabajo del
relink); el paquete de revisión nuevo se prepara por separado. Los artefactos
P5-R2 y del adaptador original permanecen congelados.

La aceptación se limita a este perfil Linux x86, este binario R3 y el caso real
observado. No certifica todos los consumidores, Family Sharing, fallo de red,
unload en caliente, GC, operación prolongada ni memoria de SRCDS bajo ASan.
HTTP streaming no se repitió en R3; su fallo anterior sigue abierto como puerta
separada de P5. Los natives, firmas de forwards e include público se conservan.
