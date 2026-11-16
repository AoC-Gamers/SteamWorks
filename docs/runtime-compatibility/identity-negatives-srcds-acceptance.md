# T15 y T29: buffers y negativos de identidad de P5-R12

2026-11-15. **T15 pasa; T29 sigue parcial.** SRCDS Linux x86 en
`competitive_dev:27015`, mediante LinuxGSM, copia `addons/sourcemod_acceptance`,
configuración de juego habitual y sin contraseña añadida. ELF R12
`839ff0751b04141031bdc6dffc48662d0804f52227f2c91015bd87c6b84cf192`.
API y fuentes de producto intactas; develop, sin commit ni producción.

## Evidencia observada

La época final del harness registra **40 comprobaciones, cero fallos, 15 errores
nativos esperados**, un cliente de prueba desconectado y una preautenticación
humana enlazada por serial con la misma entrada posteriormente autorizada.
No se exportan SteamID, tickets, direcciones de jugadores, credenciales ni logs crudos.

- Humano autorizado: texto idéntico a SourceMod y stats aceptados como control.
- Humano y bot propio: capacidades suficiente, exacta incluyendo NUL, truncada,
  5, 2, 1 y 0; conteo incluyendo NUL, terminación y canarios exteriores intactos.
- Capacidades -1 e INT_MIN: error 23 sin modificar salida. Dirección VM inválida:
  error 5. Capacidad cero precede a dirección inválida; capacidad negativa también
  se valida antes de traducir la dirección. Cliente inválido precede a capacidad.
- Bot: stats rechazados y fallback textual público/individual con AccountID cero,
  sin tratar el texto como autorización. Solo se crea y expulsa el bot del harness.
- Tras desconectarlo: identidad y stats rechazan el slot invalidado.
- Conexión humana temprana: RequestStats rechaza la referencia aún no resuelta
  con error 23. Esto **no prueba** el retorno false de un cliente conectado válido
  pero no autorizado. La misma entrada posteriormente autorizada pasa el control.
- HTTP final: status 200, cuerpo/contexto correctos, callback y cierre del handle.

Los errores se recogen con Call_Finish y continúan registrados por SourceMod;
no se suprimen comprobaciones ni logs. Desde el corte de logs anterior hay 26
excepciones esperadas: 11 de una época previa y 15 de la época final. El colector
rechaza mensajes, plugins culpados o primeros frames nativos no previstos.

## Intentos y límites

El primer harness llamó al native temprano sin capturar su excepción: los buffers
humanos pasaron, pero falló su comprobación de observación de preautenticación.
Se conserva ese intento fallido; el harness corregido captura el error y vincula
el serial antes de llamar. La época final vuelve a pasar la comprobación.

Hubo reinicios entre intentos (PID observados 112985, 114150 y 114747); no se
atribuye su causa ni se afirma continuidad entre ellos. Solo en la copia aislada
se retiraron autorrecarga y reinicio de plugins para estabilizar el observador.
La aceptación final se basa en su traza de época, no en sumar contadores anteriores.

**T29 pendiente:** cliente conectado válido no autorizado, fallback con AccountID
validado sin SteamID64 y contradicción de SteamID64/AccountID validados. Se cubren
con dobles deterministas, sin adulterar cachés de autenticación del host real.
P5 conserva seis casos abiertos: T11, T23, T25, T29, T32 y T33.

## Validación y restauración

Se repitieron stats-natives, typed-boundary y steamid-string con x86/UBSan:
32 + 34 escenarios de stats, 216 de registro/resolver, 45 de identidad, 57 HTTP/GC,
19 de formato y controles negativos de compilación. El harness compila sin warnings.
No se afirma haber repetido las 14 suites de integración ni ASan en este seguimiento.

Quit controlado final sin marcadores de crash, selector original restaurado,
DEV reconectado a Steam y laboratorio eliminado. Los hashes de CFG, extensión y
archivos originales coinciden con la captura previa. Los ZIP R12 y su reversión
permanecen intactos; el nuevo harness y esta evidencia se entregan por separado.

[Resultados y alcance](identity-negatives-srcds/results.json),
[traza final](identity-negatives-srcds/human-final.json),
[intento inicial](identity-negatives-srcds/first-attempt-human-0.json),
[restauración](identity-negatives-srcds/cleanup.json),
[matriz vigente](p5/validation-results.json).
