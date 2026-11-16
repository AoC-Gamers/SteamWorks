# Operaciones de servidor

El include público agrupa consulta de conexión Steam/VAC, estado y metadata de
servidor, identidad del servidor, reglas/key-values, red y capacidades auxiliares.
Cada native resuelve su interfaz admitida y mantiene su retorno/error ante
indisponibilidad. La presencia de un método en el SDK no garantiza capacidad
actual ni permiso para llamar sobre otro objeto.

`SteamWorks_GetPublicIP` solo acepta una dirección de familia IPv4. La composición
por octetos utiliza valores sin signo, sin depender de si `char` es signed.
El resultado de una celda conserva el patrón de bits IPv4 incluso con bit alto;
no interpretar un entero negativo como fallo de dirección por sí mismo.
IPv6 no se trunca para convertirlo en IPv4. La regresión se ejecuta con char
con signo y sin signo mediante `test-public-ip.sh`.

Los setters de metadata/reglas comunican datos al servidor Steam, no editan CFG
ni prueban que un buscador remoto ya haya actualizado su caché. Comprueba el
resultado y el estado de conexión antes de depender de esas propiedades.
Autenticación, stats, HTTP y GC tienen contratos propios en sus capítulos.
Las funciones de red expuestas siguen la interfaz legacy permitida por el registro,
no implican soporte de cualquier API Networking moderna.

El [include público](https://github.com/AoC-Gamers/SteamWorks/blob/295e274de754fb65835188f115a2fd4dd917e289/scripting/include/steamworks.inc) y
[los natives de servidor](https://github.com/AoC-Gamers/SteamWorks/blob/295e274de754fb65835188f115a2fd4dd917e289/extension/natives/gsnatives.cpp) son la referencia exacta de
firmas; no se agregan operaciones a partir de nombres de interfaz.
