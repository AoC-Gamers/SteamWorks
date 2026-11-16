# Build optimizado: aceptación en SRCDS x86

2026-11-15. **Pasa en el alcance ejecutado**, incluido el cierre limpio. Se probó
el ELF `839ff0751b04141031bdc6dffc48662d0804f52227f2c91015bd87c6b84cf192`,
producido con `--enable-optimize -O3 -DNDEBUG -m32 -Werror` tras corregir los límites
del snapshot de autenticación y del respaldo de CDetour. API pública intacta;
`develop`, sin commit, publicación ni producción.

## Entorno e identidad binaria

Se usó `competitive_dev`, LinuxGSM `/app/l4d2server`, SRCDS x86 y puerto 27015.
Solo se seleccionó temporalmente una copia en `addons/sourcemod_acceptance`, con
flags/configuraciones habituales y sin añadir contraseña. Se recompilaron cuatro
harnesses sin warnings. No se sustituyeron VM ni bibliotecas del juego/Steam.

Dispositivo, inodo y hash confirman el módulo optimizado realmente mapeado. El
PID **87189** y sus ticks de inicio se conservaron durante las dos reinicializaciones
y las pruebas humanas. Se listaron **138 plugins**, sin filas Error/Failed.
SourceTVSupport conservó el bridge del init del SDK; SteamWorks conservó el CALL
de init del engine y el detour de shutdown. [Propietarios verificados](optimized-srcds/hook-owners.json).

## Resultado observado

| Prueba | Resultado |
|---|---|
| Reinicialización completa | `map c2m2_fairgrounds` y `map c2m1_highway`, mismo proceso; tres init exitosos contando el arranque frío y un forward de conexión por reinicialización |
| HTTP y streaming | 25 completions, cero fallos del harness; texto, binario, vacío, HTTP 503, timeout, TLS inválido, conexión rechazada y streaming; 18 bytes y tres eventos headers, con ambas celdas del contexto |
| Generación y cancelación | Una respuesta retenida cruzó shutdown; se liberó tras pasar HTTP en el runtime nuevo, antes del timeout de 60 s; siete segundos de observación sin callback obsoleto, cierre seguro del handle y cancelación inmediata sin callback |
| Autenticación real | Un humano autorizado y un forward BeginAuthSession; despacho SourceMod 0 y validación Steam 0 por separado; un evento en cada forward público de validación; interfaz nativa 015 y adaptador auth 014 |
| Identidad y licencia | ID textual coincide con SourceMod, conteo con NUL, buffer pequeño terminado, capacidad cero sin sobrescritura y licencia de AppID 550 con resultado HasLicense (0) |
| Stats de solo lectura | Entero igual por cliente/AccountID; clave entera como float rechazada sin modificar sentinels. Dos recorridos de 733 claves: las mismas 12 floats finitas y coincidentes por ambas rutas. Las 16 lecturas de clave ausente fallaron preservando salidas, incluidos los bits de NaN |
| Salida | Tres shutdown observados contando el quit final; proceso detenido sin SIGSEGV, marcadores de crash, errores SourceMod ni nuevo core asociado |

Se activó `steamworks_debug 15` para lifecycle/interfaces/convivencia y después
`steamworks_debug 12` para autenticación. Se guardaron solo contadores y resultados:
sin inspeccionar/exportar tickets, AccountIDs del forward ni valores personales
de stats. El ensayo real comprueba el camino válido del ticket; los tamaños
extremos y fallos de capacidad siguen acreditados por las pruebas deterministas
del [build optimizado](optimized-build.md), sin inyectar tickets sintéticos al motor.

## Restauración y límites

LinuxGSM arrancó el DEV habitual, PID **89791**, con `addons/sourcemod1`, extensión
original mapeada y Steam conectado. Se comprobaron los hashes originales del
selector, extensión y CFG de AoC/Family Share. Se retiraron laboratorio, recovery
privado y fixture. No se aplicó un ZIP de rollback en sitio: esta restauración del
selector no se presenta como una repetición de T35.

Esta aceptación se atribuye exclusivamente al ELF optimizado `839ff075…`. El build
canónico `75a9b5d9…` no se ejecutó aquí y no hereda estos resultados. Los ZIP P5-R8
y anteriores siguen intactos, con sus hashes históricos. Queda renovar la
integración/paquetes P5 con este candidato; la matriz global conserva sus variantes
pendientes de GC/interfaces/consumidores, identidad, hot unload/owner unload,
desconexión de Steam y operación sostenida. Este ensayo no acepta toda la matriz.

## Evidencia

[Resultados y hashes](optimized-srcds/results.json),
[pruebas automáticas](optimized-srcds/automatic-checks.json),
[autenticación/identidad](optimized-srcds/checks-auth.json),
[stats, ronda 1](optimized-srcds/checks-stats-1.json),
[stats, ronda 2](optimized-srcds/checks-stats-2.json),
[módulos finales](optimized-srcds/loaded-modules-final.json),
[salida](optimized-srcds/shutdown.json),
[auditoría mínima de cores](optimized-srcds/core-audit.json) y
[restauración final](optimized-srcds/cleanup.json).

Paquete separado de revisión:
`.build/optimized-srcds/review/steamworks-optimized-linux-x86-srcds-acceptance.zip`,
con manifiesto y checksums. Incluye evidencia filtrada y fuentes de harnesses;
excluye logs crudos de LinuxGSM, configuraciones privadas, claves TLS, SDK, módulos
del juego y cores. Los cores originales de systemd se conservaron.
