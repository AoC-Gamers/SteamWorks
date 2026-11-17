# Ciclo de vida y propiedad

Inicialización solo instala hooks después de resolver interfaces y completar
los detours necesarios. La creación y habilitación de cada detour se comprueba;
un fallo revierte la pareja parcial y evita hooks dependientes. Una capacidad
opcional ausente puede reintentarse desde un frame hook sin duplicar registros.

El detour de init convive con SourceTVSupport y otros consumidores: debe observar
la llamada final y su resultado sin reemplazar una cadena ajena ni restaurar
bytes que ya no le pertenecen. La coexistencia no autoriza enganchar una ruta
sin identidad y prototipo comprobados.

Al detenerse, se invalida la generación, se retiran frame hooks y hooks por sus
IDs, se cancelan call results/callbacks y se liberan forwards y requests propios.
Las llamadas repetidas son inertes. Callbacks de una generación vieja se ignoran.
Si el plugin propietario se descarga con HTTP pendiente, no recibe llamadas
posteriores. Cerrar un request en su callback difiere su destrucción hasta salir.

El rol cliente Steam del proceso solo se finaliza tras observar parada del
GameServer y comprobar que su pipe ya no sigue activo; una descarga ordinaria
no debe cerrar una sesión del host. Una reinicialización válida cancela la
observación de parada anterior. Esto evita el teardown desequilibrado observado
en SRCDS sin apropiarse de interfaces prestadas.

Carga tardía, descarga/recarga soportada y reinicialización completa en el mismo
proceso son escenarios diferentes. Las suites `test-resource-lifecycle.sh`,
`test-detours.sh` y `test-init-coexistence.sh` comprueban orden, fallos y reentrada;
los harnesses SRCDS requieren operación explícita. No fuerces una descarga con
consumidores que todavía dependan de la extensión.

## Una carga propietaria por módulo

SourceMod puede resolver el nombre lógico `steamworks` y el nombre físico del
artefacto al mismo módulo, conservando entradas distintas. SteamWorks admite un
solo propietario de carga: el factory `GetSMExtAPI` rechaza otro intento antes
de que el cargador vuelva a enlazar Metamod o cambie la identidad y los recursos
del propietario activo. Un rechazo no descarga ni reinicializa esa instancia.

La protección cubre inicialización, funcionamiento y desmontaje hasta que
SourceMod termina de retirar las dependencias. Un fallo de inicialización libera
la reserva para reintentar; una descarga completa permite una nueva carga.
Utiliza `sm exts load steamworks`. Si se cargó primero por el nombre físico,
retira sus consumidores y descarga esa entrada antes de cargar el nombre lógico.
Si SourceMod conservó una entrada fallida al resolver dependencias, retírala
también antes de reintentar. La segunda entrada se rechaza; no actúa como alias
para satisfacer dependencias.

Las pruebas locales x86 ejecutan el factory y los wrappers de producción con un
host controlado, ASan/UBSan y un control negativo sin protección del factory.
Se realizaron pruebas locales en SRCDS Linux x86 con ambos órdenes de carga:
los intentos duplicados se rechazaron sin alterar la biblioteca ni la ConVar del
propietario, y las peticiones HTTP pendientes completaron con contexto y cuerpo
correctos. También se comprobó la recuperación del consumidor tras una carga
fallida, la descarga/recarga completa en el mismo proceso y el cierre limpio.
La atribución del binario se conserva en la matriz de cobertura. Estos ensayos
de carga no acreditan runtime Windows ni callbacks de stats; los resultados
propios de stats y sus límites figuran en su [contrato](../stats/README.md).
