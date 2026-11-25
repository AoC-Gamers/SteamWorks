# Compatibilidad y alcance

El entorno runtime acreditado es L4D2 dedicado Linux x86 con SourceMod/Metamod y
los módulos identificados por los perfiles nativos de los builds de Steam
`23990100` y `26770943`. Ambos perfiles requieren el mismo `libsteam_api.so`
verificado y la llamada de inicialización propia de cada motor. Windows dispone de compilación
y controles offline, pero no de aceptación SRCDS equivalente. Los sondeos x64
históricos no acreditan el motor x86. Un perfil JSON documental no habilita ABI.

## Migraciones de consumidores

- El segundo argumento de `RequestStats` y `RequestStatsAuthID` es el AppID del
  contexto, **no universo**. Las firmas se conservan; los consumidores antiguos
  que pasaban universo deben corregir su interpretación.
- El AuthID de una celda representa AccountID de 32 bits, no SteamID64 truncado.
- `GetClientSteamID` mantiene su conteo textual histórico que incluye NUL cuando
  hay capacidad. El helper HTTP `written` excluye NUL.
- El helper HTTP textual requiere cinco argumentos; la llamada antigua de tres
  se rechaza antes de leer salidas ausentes. RawData exige el tamaño completo.
- SDK 1.55 y el motor usan versión 014. El prefijo auditado de autenticación
  y la interfaz SDK completa conservan tipos y rutas de resolución independientes;
  no se convierte uno en otro por cast. La versión 015 se rechaza.

## Límites de aceptación

La API GC se conserva por compatibilidad y depende del soporte del juego y del
contexto Steam. Consulta las [dependencias funcionales de GC](../game-coordinator/README.md#aplicabilidad-a-l4d2-dedicado-x86).
Las suites locales comprueban ABI, buffers, forwards y ciclo de vida mediante
proveedores controlados. Runtime Windows continúa pendiente.
`ISteamFriends` no está expuesta; su resultado nulo observado pertenece al
contexto dedicado ensayado, no a todos los procesos Steam.

Los ensayos históricos mínimos con SourceTVSupport conservaron su atribución.
Un ensayo posterior reprodujo el SIGSEGV de salida del baseline archivado:
un hilo Steam accedió a la interfaz de crashhandler después de descargar su
módulo y el manejador de señal también quedó inválido. Ese baseline carece
del finalizador de la sesión cliente Steam. Un baseline nuevo SDK 1.55 con ese finalizador
pasó cualificación y reversión con cierre limpio en el perfil mínimo. No se
declara resuelto universalmente: la pila completa exige su propia aceptación.
No trasladar resultados entre hashes distintos por compartir nombre de candidato.

El catálogo de cobertura de las fuentes conserva fecha y hash por observación,
con nombres genéricos para las instancias. Los resultados históricos pertenecen
a su payload y no se presentan como una ejecución nueva ni una aceptación global.

## Cambio de SDK

El único SDK de compilación/enlace es 1.55, validado por contenido mediante el
lock de fuentes. No se transfieren los ensayos históricos de otros payloads al
binario nuevo. El payload SDK 1.55 con SHA256
`abb4d03a47f88315950ba8a63c2a1696d8daa80cf52039c808e95569f6adad52`
fue observado en SRCDS Linux x86 el 2026-11-16: autenticación 014 con cuenta
autorizada, identidad, stats enteros y float, HTTP/streaming y reinicialización
en el mismo proceso con petición pendiente, seguido de cierre limpio.
El ensayo mínimo incluyó la extensión SourceTVSupport; no acredita su VSP
ni emisión, toda la pila de plugins o runtime Windows.
La atribución completa se conserva en el catálogo de cobertura de las fuentes.

El baseline nuevo de reversión tiene SHA256
`678e334bcfae091461cf43b10ed6d487780fc34d354b9ad7c4b0826747e71bbd`.
Usa el mismo snapshot de fuentes del candidato, compilado sin optimización;
no representa volver a una versión funcional anterior. La aceptación en sitio
corresponde al par ZIP ensayado; regenerar documentación cambia el ZIP completo,
aunque ELF/include permanezcan idénticos. El baseline histórico fallido conserva
su identidad y no queda corregido por este ensayo.

Los ensayos posteriores del mismo par de payloads confirmaron instalación y
reversión desde sus respectivos ZIP, con tres cierres limpios por ensayo. Sus
trazas observaron el contexto cliente activo antes de finalizarlo y cerrado
después, con GameServer detenido. Esas mediciones corresponden a los ensayos
identificados en la cobertura de fuentes; no se atribuyen a los ciclos
anteriores donde el pipe no se midió ni a los ZIP regenerados posteriormente.
