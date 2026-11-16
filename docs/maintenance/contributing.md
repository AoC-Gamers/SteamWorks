# Contribuir

Mantén las firmas y opcionalidad de `scripting/include/steamworks.inc`. Audita
cualquier cambio de SDK, interfaz o ABI antes de ampliar una allowlist. No uses
una modificación documental como autorización para operar un servidor.

Consulta [build](../build/README.md), [contratos](../README.md) y el catálogo
`tests/README.md` de las fuentes. Ejecuta integración en una carpeta de resultados
nueva. Conserva controles negativos y las suites opcionales cuando dispongas de
sus dependencias; si faltan, informa pendiente, sin convertirlo en éxito.

Una contribución debe distinguir cambios funcionales, comprobaciones locales y
aceptación de runtime con hash. No vuelvas a versionar diarios o ZIP generados en
`docs/`. Nunca distribuyas SDK, claves, tickets, VM diagnóstica ni fixtures como
parte del producto. Véase [política documental](documentation.md).
