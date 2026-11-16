# Diagnósticos x86 sin inicializar un gameserver

`http_stream_context_probe.cpp` observa el contexto binario de headers/data y
completion en un proveedor real, con contexto sintético. Crea un usuario/pipe
local; no llama a GameServer_Init, logon ni métodos de autenticación. El runner
levanta solamente la fixture HTTP de loopback. Ejecutar en un contenedor
`--network none`, HOME y cwd temporales, con el checkout montado en `/workspace`:

```sh
g++ -m32 -std=c++14 -Wall -Wextra -Werror -isystem sdk_158a/public/steam \
  tests/diagnostics/http_stream_context_probe.cpp -ldl -o .build/http-stream-context/provider-probe
# Dentro del contenedor aislado, desde /tmp y con HOME=/tmp:
python3 /workspace/tests/diagnostics/run_http_stream_context_probe.py \
  /workspace/.build/http-stream-context/provider-probe /ruta/copia/steamclient.so
```

La sonda requiere SteamClient020/HTTP003 y devuelve error si faltan los eventos,
el tamaño esperado, los seis bytes o el completion correcto. Imprime únicamente
los eventos sintéticos. El resultado no reemplaza las pruebas deterministas de
`scripts/test-http-stream-context.sh` ni la aceptación en SRCDS; consultar
`docs/runtime-compatibility/http-stream-context.md`.

`extension_load_probe.cpp` reproduce la carga local de una extensión con
`RTLD_NOW | RTLD_LOCAL`. Compilar para x86 y usar los proveedores de la instalación
que se quiere validar:

```sh
g++ -m32 -std=c++14 -Wall -Wextra -Werror \
  tests/diagnostics/extension_load_probe.cpp -ldl -o /tmp/extension-load-probe
LD_LIBRARY_PATH=/ruta/serverfiles/bin /tmp/extension-load-probe /ruta/steamworks.ext.so
```

El proceso no llama a interfaces de la extensión, no inicializa Steam y no hace
una autenticación. Detecta imports no resueltos y rutas de SDK incrustadas en las
dependencias. No sustituye la aceptación dentro de SourceMod/SRCDS. Mantiene las
bibliotecas mapeadas hasta terminar el proceso.

`gameserver_interface_probe.cpp` compara objetos/vtables 014 y 015 en un proceso
x86 separado usando SteamClient020, usuario y pipe locales; tampoco llama a
métodos de autenticación. Su evidencia corresponde al diagnóstico anterior al
adaptador y está documentada en `docs/runtime-compatibility/auth-debug.md`.
