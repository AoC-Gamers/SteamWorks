# SteamWorks para Left 4 Dead 2

Extensión SourceMod que permite a plugins SourcePawn utilizar autenticación,
identidad, stats, HTTP y capacidades del servidor Steam. Derivada de
[KyleSanderson/SteamWorks](https://github.com/KyleSanderson/SteamWorks), mantenida
por AoC Gamers. Licencia: [GPL](LICENSE), con las excepciones indicadas en las fuentes.

Necesita SourceMod, Metamod:Source y un runtime Steam compatible. La ruta acreditada
es **L4D2 dedicado Linux x86**; compilar Windows no acredita su runtime y los
sondeos x64 históricos no son soporte para el motor x86 de L4D2. Steamworks SDK 1.55 es la única
dependencia Steam de compilación, no un runtime que deba copiarse al servidor.

Para compilar, obtén **Steamworks SDK 1.55 (`sdk_155`)** desde el
[portal oficial de Steamworks](https://partner.steamgames.com/doc/sdk). Consulta
[obtención y validación del SDK](docs/build/dependencies.md#obtener-steamworks-sdk-155):
el enlace a la última versión no garantiza que descargue la 1.55.

Las fuentes instalables siguen el árbol `addons/sourcemod/`; el código C++
permanece en `extension/` y los diagnósticos en `tests/`.

Instala la extensión en `addons/sourcemod/extensions/` y el include en
`addons/sourcemod/scripting/include/`. Actualiza con el servidor detenido y una
copia de los archivos propios para poder revertir. No instales los fixtures ni
la VM diagnóstica. Sigue la [guía de instalación](docs/installation/README.md).

- [Documentación por funcionalidad](docs/README.md)
- [Compilación y dependencias](docs/build/README.md)
- [Compatibilidad y límites](docs/runtime/compatibility.md)
- [Pruebas y cobertura](https://github.com/AoC-Gamers/SteamWorks/tree/develop/tests)

La API de Game Coordinator se conserva por compatibilidad. Su funcionamiento
depende del soporte del juego y del contexto Steam. Consulta los
[límites de GC](docs/game-coordinator/README.md#aplicabilidad-a-l4d2-dedicado-x86).
