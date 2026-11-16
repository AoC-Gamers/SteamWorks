# Compatibilidad y alcance

El entorno runtime acreditado es L4D2 dedicado Linux x86 con SourceMod/Metamod y
los módulos identificados por el perfil nativo. Windows dispone de compilación
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
- El hook de autenticación del motor usa 014; los natives de servidor usan 015.
  No son punteros intercambiables.

## Límites de aceptación

**GC remoto positivo sigue pendiente.** Una cola vacía o un proveedor local
permiten validar buffers/forwards, no el protocolo remoto ni mensajes del backend.
`ISteamFriends` no está expuesta; su resultado nulo observado pertenece al
contexto dedicado ensayado, no a todos los procesos Steam.

La instalación/reversión histórica mínima con SourceTVSupport tuvo salidas
limpias. El SIGSEGV de un baseline anterior no se reprodujo allí y no se declara
resuelto universalmente. La pila completa de plugins exige su propia aceptación.
No trasladar resultados entre hashes distintos por compartir nombre de candidato.

El catálogo de cobertura de las fuentes conserva fecha, revisión y hash por
observación. La [integración histórica fijada](https://github.com/AoC-Gamers/SteamWorks/blob/295e274de754fb65835188f115a2fd4dd917e289/docs/runtime-compatibility/p5-integration.md)
explica el alcance del payload ELF `72664c65…`; no es una ejecución nueva.
