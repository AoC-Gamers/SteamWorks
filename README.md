# SteamWorks para Left 4 Dead 2

Extensión SourceMod que permite a plugins SourcePawn utilizar autenticación,
identidad, stats, HTTP y capacidades del servidor Steam. Derivada de
[KyleSanderson/SteamWorks](https://github.com/KyleSanderson/SteamWorks), mantenida
por AoC Gamers. Licencia: [GPL](LICENSE), con las excepciones indicadas en las fuentes.

Necesita SourceMod, Metamod:Source y un runtime Steam compatible. La ruta acreditada
es **L4D2 dedicado Linux x86**; compilar Windows no acredita su runtime y los
sondeos x64 históricos no son soporte para el motor x86 de L4D2. Steamworks SDK 1.55 es la única
dependencia Steam de compilación, no un runtime que deba copiarse al servidor.

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

La recepción positiva desde el GC remoto sigue pendiente; un proveedor local o
una cola vacía no la demuestran.
