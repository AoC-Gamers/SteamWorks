# P1-E: conteo de GetClientSteamID y buffers límite

Fecha: 2026-11-14. Rama: `develop`. Base
`46e3ee05fc29ad090a1d6f7b1cb5e3dca9855350`, con P1-A/B/C/D y documentación locales
conservadas. Sin commit, release ni despliegue. H09 conciliado en código y
documentación, con regresión unitaria; aceptación en servidor pendiente.

## Decisión de compatibilidad

La firma pública se conserva:

```sourcepawn
native int SteamWorks_GetClientSteamID(int client, char[] steamId, int length);
```

El comentario anterior excluía el terminador, pero el código añadía uno al
resultado de `ISourceMod::Format`. Se conserva el **conteo histórico que incluye
NUL para capacidades positivas válidas**, y se corrigen el include y el README.
No se cambia silenciosamente el retorno de esos llamadores a `strlen`.

Se revisaron el contrato y la implementación del formateador: `Format` delega en
`FormatArgs`, que llama a `ke::SafeVsprintf`. AMTL devuelve los bytes de texto
realmente almacenados, excluyendo el terminador; al truncar devuelve capacidad
menos uno. P1-E conserva la suma de uno del native sobre ese contrato.

| Entrada válida | Escritura y retorno |
| --- | --- |
| Capacidad suficiente | Texto completo terminado en NUL; retorno `strlen(texto) + 1` |
| Buffer pequeño positivo | Prefijo terminado en NUL; retorno de bytes almacenados incluyendo NUL, no longitud requerida |
| Capacidad 1 | Solo NUL; retorno 1 |
| Capacidad 0 | Ninguna escritura ni resolución del destino; retorno 0 |

Una capacidad negativa genera error nativo antes de convertirse a `size_t` o
invocar el formateador. Un error de `LocalToString` se propaga mediante
`ThrowNativeErrorEx` antes de construir la identidad o formatear. La precedencia
es cliente conectado, longitud y dirección de salida. Cliente inválido o
desconectado conserva el error nativo anterior, incluso con longitud cero.

El cambio observable de límite es capacidad cero: antes podía devolver 1 pese
a no haber almacenado terminador; ahora devuelve 0. No se conserva ese conteo
ficticio. Los errores de longitud/dirección quedan controlados en vez de
continuar con una conversión o puntero inválidos.

## Revisión de consumidores

Se repitió el inventario de fuentes `.sp/.inc` bajo el directorio local `GitHub`,
con las exclusiones de dependencias/build/SDK descritas en P0. El único caller
encontrado, fuera de declaraciones, es
[steamwork_test.sp](../../scripting/steamwork_test.sp), comando `sm_swtest_identity`:

- Proporciona un buffer de 64 bytes con `sizeof` como capacidad.
- Registra el retorno y la cadena; considera disponible el texto si el retorno
  es positivo y el primer carácter no es NUL.
- No exige una longitud exacta ni calcula un offset a partir del retorno.

No requiere migración para el conteo conservado. El harness se recompiló con el
include actualizado sin warnings.

Los otros hallazgos son declaraciones/copias de include en AoC, BanSystem,
Connect, Family Share y L4D1_2-Plugins. No se modificaron otros repositorios.
El barrido léxico no prueba ausencia de llamadas indirectas, plugins solo
precompilados, servidores remotos o fuentes fuera del directorio examinado.
Conservar el retorno histórico positivo reduce el cambio para esos consumidores
no observados; no constituye una prueba de compatibilidad runtime universal.

## Cambios acotados

En [gsnatives.cpp](../../extension/natives/gsnatives.cpp) solo cambia el cuerpo
de `sm_GetClientSteamID` respecto al estado P1-D. La capacidad se valida antes de
convertirla a tamaño sin signo, se comprueba la conversión de dirección y se usa
`size_t` para el conteo del formateador antes de convertir el resultado público.
Con el contrato de `Format`, el retorno positivo no supera la capacidad positiva
de la celda de entrada.

Los nombres, orden/tipos de argumentos, enums y registros no cambian. La
construcción legacy de identidad mantiene los índices 4/5 de sus opciones
internas; no se reforma su política de autenticación ni se cambia a la política
de `RequestStats` de P1-B. Los cuerpos de IP pública y los demás natives
GameServer permanecen idénticos a P1-D.

La capacidad positiva debe corresponder al buffer real que aporta el llamador.
Esta entrega comprueba longitud firmada y dirección base, no incorpora una
validación general de todo el rango de memoria de un buffer en la VM.

## Pruebas deterministas con formateador real

[steamid_string_test.cpp](../../tests/steamid_string_test.cpp) invoca el native
de producción desde su tabla. Usa el `CSteamID` real del SDK y un adaptador de
host que delega en **`ke::SafeVsprintf` real**, no en una copia de su algoritmo ni
en retornos prefabricados. SourceMod, clientes, resolución de memoria y el
constructor legacy de identidad se controlan con dobles; el significado de la
identidad construida no forma parte de esta suite de conteo.

La implementación de AMTL usada en host y contenedor coincide con la revisión
`2d3b1a3378a3728637f26660c9ffc2df3189cf62` y la misma huella de `am-string.h`.
Se contrastó también la delegación `Format` → `FormatArgs` → `SafeVsprintf` en
el SourceMod fijado del contenedor P0.

Los **19 casos** comprueban:

- Buffer suficiente, capacidad exacta, un byte menos y buffers de 5/2/1 bytes.
- Texto esperado, terminador y todos los bytes exteriores como canarios.
- Tamaño cero, -1 y mínimo de celda firmada, sin llamada al formateador.
- Dirección inválida, sin formateo ni construcción de identidad.
- Precedencia cliente → longitud → dirección y causa del error.
- Formato de 64 bits en sus límites de 20 dígitos con buffer exacto, truncado y
  mínimo. `UINT64_MAX` se usa como frontera de formato, no como usuario válido.
- Posiciones legacy de identidad conservadas y una única llamada al formateador
  en los casos válidos.

Cada caso usa expectativas de texto y conteo literales. Los buffers físicos de
la fixture tienen capacidad suficiente para cada tamaño positivo probado.

Contra el C++ local anterior a P1-E, los nueve controles de capacidad positiva
siguen pasando con las mismas expectativas. El caso de capacidad cero falla de
forma determinista por el retorno 1 anterior. No se ejecutaron capacidades
negativas ni direcciones inválidas contra el código inseguro anterior.

## Resultados y pendientes

| Prueba / nivel | Resultado |
| --- | --- |
| T15/U | 19 casos de conteo, terminación, truncamiento y límites aprobados en x86_64 y x86 con UBSan |
| T28/U, solo SteamID textual | Errores de dirección y precedencia comprobados con dobles |
| T25/B, alcance local | Caller revisado y harness recompilado; comportamiento positivo conserva nueve controles del código anterior |
| T24/B | Superficie pública equivalente a P0; cambio del include solo documental |
| Regresiones P1-A/B/C/D | 152 ejecuciones previas por arquitectura siguen pasando, incluyendo ambos signos de char de IP |
| Headers reales | `gsnatives.cpp` compila sin warnings como objeto ELF32 i386 |
| CI Linux | Suite incorporada usando AMTL de las dependencias; lint local aprobado, ejecución remota pendiente |
| T15/I, T25/I y T28/I | Pendientes de una instalación SourceMod/L4D2 identificada y plugins precompilados/recompilados |

Con esta suite son 171 ejecuciones de escenario por arquitectura, distinguiendo
las dos variantes de signo de char de P1-D. UBSan no sustituye validación de
memoria de una VM real ni pruebas de carga de plugins.

La compilación con headers reales usa el procedimiento diagnóstico de fases
anteriores: omite solo la opción de C `-Wno-pointer-to-int-cast` del comando GCC
registrado, conserva `-Werror` y no modifica AMBuild. Sigue sin ser un build
completo enlazado; su bloqueo anterior permanece registrado.

## Reproducción, evidencia y reversión

```bash
AMTL_DIR=/ruta/a/sourcemod/public/amtl bash scripts/test-steamid-string.sh
```

Por defecto el runner busca AMTL en `public/amtl` bajo `SOURCEMOD_DIR`, o en
`.deps/exts-linux/sourcemod-1.12/` si no se indica ese directorio. Acepta también
`STEAMWORKS_SDK_DIR`, `CXX`, `TEST_BUILD_DIR` y `TEST_CXXFLAGS`. Para x86 se usó
`-m32` en el contenedor P0, sin red ni instalaciones nuevas.

El [registro JSON](p1-e/validation-results.json) identifica fuentes actuales y
anteriores, consumidores, dependencias, comandos y resultados. Los logs y
objetos locales están en `.build/p1-e/`; se conservan las evidencias de fases
anteriores como instantáneas históricas.

Para revertir solo P1-E, retirar su diff de SteamID textual en `gsnatives.cpp`,
comentarios del include, extensiones de la fixture compartida, nueva suite,
runner, paso CI y documentación, conservando P1-D y fases previas. No hubo
despliegue que revertir. La aceptación operativa de H09 sigue pendiente.
