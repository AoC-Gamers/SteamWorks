# Documentación

Los capítulos describen contratos implementados. La cobertura distingue pruebas
locales y observaciones de runtime; no extiende aceptación a otros binarios.

| Área | Entrada |
|---|---|
| Instalación y reversión | [Instalación](installation/README.md) |
| Compilación | [Build](build/README.md), [dependencias](build/dependencies.md), [CI](build/ci.md), [SonarQube](build/sonar.md) |
| Arquitectura | [Responsabilidades](architecture/README.md), [frontera tipada](architecture/typed-boundary.md) |
| Runtime | [Interfaces](runtime/interfaces.md), [ciclo de vida](runtime/lifecycle.md), [compatibilidad](runtime/compatibility.md) |
| Autenticación | [Contrato](authentication/README.md), [adaptador 014](authentication/gameserver-014.md) |
| Identidad | [AccountID y SteamID](identity/README.md) |
| Servidor | [Estado, metadata e IPv4](server/README.md) |
| Stats | [Solicitudes y lectores](stats/README.md), [schema](stats/schema.md) |
| HTTP | [Buffers](http/README.md), [callbacks](http/callbacks.md), [streaming](http/streaming.md) |
| Game Coordinator | [Mensajes y forward binario](game-coordinator/README.md) |
| Diagnóstico | [Debug](diagnostics/README.md), [problemas conocidos](diagnostics/troubleshooting.md) |
| Mantenimiento | [Contribuir](maintenance/contributing.md), [documentar](maintenance/documentation.md) |

El [catálogo de pruebas](https://github.com/AoC-Gamers/SteamWorks/tree/develop/tests)
se mantiene en las fuentes y no se distribuye con el paquete de instalación.
