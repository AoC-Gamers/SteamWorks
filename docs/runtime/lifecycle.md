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
