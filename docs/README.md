# Documentación técnica

Este directorio reúne notas técnicas sobre comportamientos específicos del runtime,
limitaciones observadas en servidores dedicados y detalles prácticos de uso de la
extensión.

Documentos actuales:

- [DEVELOPMENT](DEVELOPMENT.md)
- [BUILD_SYSTEM](BUILD_SYSTEM.md)
- [GetSchemaForGame_550](GetSchemaForGame_550.md)
- [ISteamHTTP](ISteamHTTP.md)
- [Corrección del contexto HTTP streaming en x86](runtime-compatibility/http-stream-context.md)
- [Aceptación del contexto HTTP streaming en SRCDS x86](runtime-compatibility/http-stream-srcds-acceptance.md)
- [ISteamFriends](ISteamFriends.md)
- [ISteamUserStats](ISteamUserStats.md)
- [TestCoverage](TestCoverage.md)
- [P0: inventario de API, consumidores, SDK y pruebas](runtime-compatibility/p0-baseline.md)
- [P1-A: corrección y pruebas de salidas de stats](runtime-compatibility/p1-a-stats.md)
- [P1-B: AppID e identidad en solicitudes de stats](runtime-compatibility/p1-b-stats-identity.md)
- [P1-C: creación y callbacks de detours](runtime-compatibility/p1-c-detours.md)
- [P1-D: validación de IPv4 y signo de char](runtime-compatibility/p1-d-public-ip.md)
- [P1-E: conteo y buffers de GetClientSteamID](runtime-compatibility/p1-e-steamid-count.md)
- [P2: frontera tipada y registro de interfaces](runtime-compatibility/p2-typed-interfaces.md)
- [P3: ciclo de vida y propiedad de recursos](runtime-compatibility/p3-resource-lifecycle.md)
- [P4: evidencia binaria y perfiles mínimos](runtime-compatibility/p4-binary-profiles.md)
- [Aceptación del adaptador 014 en SRCDS x86](runtime-compatibility/auth014-srcds-acceptance.md)
- [BeginAuthSession: adaptador tipado SteamGameServer014](runtime-compatibility/begin-auth-session-014.md)
- [BeginAuthSession y convar de debug](runtime-compatibility/auth-debug.md)
- [Reintento SRCDS con jugador real](runtime-compatibility/srcds-acceptance-retry.md)
- [Aceptación SRCDS de P5-R2: resultados y límites](runtime-compatibility/srcds-acceptance.md)
- [P5: integración, resultados y artefactos locales](runtime-compatibility/p5-integration.md)
- [P5-R3: autenticación, identidad y stats en SRCDS x86](runtime-compatibility/p5-r3-auth-srcds-acceptance.md)
- [P5-R6: autenticación, identidad y stats históricos en SRCDS x86](runtime-compatibility/p5-r6-auth-srcds-acceptance.md)
- [P5-R4: floats positivos y claves inexistentes en SRCDS x86](runtime-compatibility/p5-r4-stats-srcds-acceptance.md)
- [P5-R7: reversión en sitio y arranque; fallo de salida del baseline conservado](runtime-compatibility/p5-r7-rollback-srcds-acceptance.md)
- [P5-R5: reversión en sitio, arranque y fallo comparativo de salida](runtime-compatibility/p5-r5-rollback-srcds-acceptance.md)
- [T36: marshalling binario del forward de autenticación](runtime-compatibility/t36-auth-forward.md)
- [T08: causa y corrección del SIGSEGV al salir de SRCDS x86](runtime-compatibility/t08-srcds-shutdown.md)
- [T08: aceptación fallida de reinicialización con SourceTVSupport](runtime-compatibility/t08-reinit-srcds-acceptance.md)
- [T08: corrección local de convivencia del init con SourceTVSupport](runtime-compatibility/t08-init-coexistence.md)
- [T08: reinicialización aceptada con SourceTVSupport en SRCDS x86](runtime-compatibility/t08-coexist-srcds-acceptance.md)
- [SourcePawn: causa y corrección del fallo de destrucción bajo ASan](runtime-compatibility/sourcepawn-vm-asan.md)
- [Arquitectura y contratos integrados](runtime-compatibility/architecture.md)
- [Registro de compatibilidad y migración H08/H09](runtime-compatibility/compatibility.md)
- [T25 de R15: evidencia inicial parcial de consumidores y H08/H09](runtime-compatibility/consumer-compat-srcds-acceptance.md)
- [T25/T29: cierre del perfil Linux x86 y corrección de Connect](runtime-compatibility/last-two-srcds-progress.md)
- [P5-R18: correcciones de CI Windows, integración y artefactos renovados](runtime-compatibility/p5-integration.md)
- [Preparación y reversión de P5](runtime-compatibility/p5-rollback.md)

Objetivo de estos documentos:

- dejar evidencia de pruebas reales contra el runtime de L4D2
- documentar diferencias entre lo que el SDK expone y lo que el gameserver permite usar
- evitar reintroducir implementaciones que ya fueron probadas y descartadas

- [Corrección del build x86 optimizado: tickets y límites de CDetour](runtime-compatibility/optimized-build.md)

- [Aceptación del build optimizado en SRCDS x86](runtime-compatibility/optimized-srcds-acceptance.md)

- [P5-R9: T26/T35, instalación y reversión en sitio aceptadas](runtime-compatibility/p5-r9-rollback-srcds-acceptance.md)

- [T20: propietario descargado con HTTP pendiente en SRCDS x86](runtime-compatibility/t20-owner-srcds-acceptance.md)

- [T15/T29: buffers y negativos de identidad en SRCDS x86](runtime-compatibility/identity-negatives-srcds-acceptance.md)
- [T11: matriz de interfaces/capacidades y GC en SRCDS x86](runtime-compatibility/interface-capabilities-srcds-acceptance.md)
- [T23: consumidores y resultados de forwards en SRCDS x86](runtime-compatibility/forward-consumers-srcds-acceptance.md)
- [T16: carga tardía y descarga/recarga soportada en SRCDS x86](runtime-compatibility/t16-extension-srcds-acceptance.md)
