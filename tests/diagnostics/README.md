# Probes diagnósticos

Las tres fuentes C++ de este directorio investigan carga de extensión, interfaces
y el layout de callbacks HTTP. `run_http_stream_context_probe.py` automatiza el
proveedor HTTP local para ese sondeo. Son herramientas de investigación, no suites
de aceptación ni extensiones instalables.

Revisa sus argumentos, arquitectura, biblioteca y destino antes de ejecutarlas;
pueden cargar bibliotecas nativas y el sondeo HTTP puede abrir conexiones. No se
invocan automáticamente desde integración. Conserva resultados en `.build/`,
identifica binarios por hash y nunca registres tickets o claves.
La regresión determinista del contexto vive en `../native/http_stream_context_test.cpp`
y `../runners/test-http-stream-context.sh`, con wire independiente y
control negativo. No sustituirla por un probe del mismo header SDK.

El probe de GameServer solicita solo la versión 014 de SDK 1.55 e inspecciona
el slot de autenticación sin invocarlo; no exige que exista una interfaz 015.
