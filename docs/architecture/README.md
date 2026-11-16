# Arquitectura

SourcePawn invoca los natives del include público. Los módulos C++ validan los
argumentos, convierten identidades/longitudes en tipos internos y resuelven la
capacidad necesaria. Las operaciones asíncronas regresan después por callbacks,
si el propietario y la generación siguen vigentes.

El diagrama resume esa frontera; su fuente son `extension/core/`,
`extension/http/` y los registros de natives. No representa servicios nuevos.

```mermaid
flowchart TD
 A[Plugin SourcePawn] --> B[Native y validacion de argumentos]
 B --> C[Conversiones y tipos internos]
 C --> D[Resolver interfaz y capacidad]
 D --> E{Version admitida y disponible?}
 E -->|No| F[Resultado o error definido por el native]
 E -->|Si| G[Operacion Steam]
 G --> H{Asincrona?}
 H -->|No| I[Convertir resultado]
 H -->|Si| J[Callback posterior]
 J --> K[Comprobar vigencia y despachar]
 I --> L[Plugin SourcePawn]
 K --> L
```

Validar una dirección base SourcePawn **no prueba toda la capacidad anunciada**.
El plugin debe reservar memoria real suficiente; no se puede prometer protección
frente a una capacidad falsa a partir de la conversión de un puntero.

La extensión es dueña de hooks, detours, suscripciones y requests que crea;
no de las interfaces Steam prestadas por el host. El [ciclo de vida](../runtime/lifecycle.md)
evita usar recursos de una generación anterior. La aceptación de una petición
asíncrona no significa datos disponibles. Consulta [tipos](typed-boundary.md),
[HTTP](../http/README.md) y [GC](../game-coordinator/README.md).
