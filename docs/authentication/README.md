# Autenticación

El lector heredado de `extension/steamtools/ticket.h` no se utiliza para procesar
`BeginAuthSession`. Sus propietarios de memoria son no copiables y sus campos se
inicializan antes de cualquier lectura; las pruebas locales usan solo tickets
sintéticos y prefijos truncados. Esto no acredita el formato de tickets actuales
ni añade una nueva ruta de autenticación o inspección de tickets reales.

La extensión observa el `BeginAuthSession` del GameServer del motor y expone los
forwards de autenticación declarados en `steamworks.inc`. El ticket es binario:
no tratarlo como string NUL ni registrar su contenido. Los IDs y resultados se
marshallean según el include; conservar el orden y la anchura de las celdas.

El hook valida longitudes y asigna un snapshot con espacio para la representación
Pawn antes de despachar. No confundir capacidad, bytes válidos y redondeo a
celdas. La prueba con VM real comprueba el marshalling y la agregación del forward;
un host simulado por sí solo no reproduce las reglas de SourceMod.

Los forwards de validación `SW_OnValidateClient` y `SteamWorks_OnValidateClient`
conservan su contrato legacy/actual y el orden propietario/cliente. Observar una
solicitud no es autorización final: el resultado llega después por Steam y debe
pertenecer a la generación vigente. Detenerse durante un despacho suprime los
siguientes consumidores que ya no sean válidos.

Consulta el [adaptador 014](gameserver-014.md), el [debug](../diagnostics/README.md)
y la [identidad](../identity/README.md). Los tests de binding usan SourceHook
real con proveedor Steam simulado; una cuenta autenticada en SRCDS es evidencia
adicional y requiere un ensayo explícito, sin registrar tickets ni IDs personales.
