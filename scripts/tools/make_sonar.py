"""Local Cppcheck and explicit SonarScanner entrypoints, invoked through Make."""
import argparse
import base64
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile
from urllib.parse import urlsplit
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
SETTINGS = {"SONAR_HOST_URL", "SONAR_PROJECT_KEY", "SONAR_PROJECT_NAME",
            "SONAR_QUALITY_GATE_WAIT", "SONAR_QUALITY_GATE_TIMEOUT",
            "SONAR_COMPILE_COMMANDS"}


class SonarError(Exception):
    pass


def configuration(root):
    # Read values as data, never source shell commands. Explicit environment wins.
    env = os.environ.copy()
    for name, allowed in ((".env", SETTINGS), ("env.secret", {"SONAR_TOKEN"})):
        path = root / name
        if not path.is_file():
            continue
        for line in path.read_text(encoding="utf-8").splitlines():
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, value = (part.strip() for part in line.split("=", 1))
            if name == ".env" and key == "SONAR_TOKEN":
                raise SonarError("SONAR_TOKEN debe estar en env.secret o en el entorno.")
            if key in allowed:
                if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'":
                    value = value[1:-1]
                env.setdefault(key, value)
    return env


def scanner_context(env):
    base = env.get("SONAR_HOST_URL", "").strip().rstrip("/")
    url = urlsplit(base)
    if (url.scheme not in {"https", "http"} or not url.hostname
            or url.username is not None or url.password is not None or url.query or url.fragment):
        raise SonarError("Configura SONAR_HOST_URL con una URL sin credenciales ni query.")
    if url.scheme == "http" and url.hostname not in {"localhost", "127.0.0.1", "::1"}:
        raise SonarError("Usa HTTPS para SonarQube; HTTP solo se admite en loopback.")
    if not env.get("SONAR_TOKEN", "").strip():
        raise SonarError("Falta SONAR_TOKEN en env.secret o en el entorno.")
    wait = env.get("SONAR_QUALITY_GATE_WAIT", "true")
    timeout = env.get("SONAR_QUALITY_GATE_TIMEOUT", "300")
    if wait not in {"true", "false"} or not timeout.isdecimal() or int(timeout) < 1:
        raise SonarError("La espera del gate debe ser true/false y su timeout un entero positivo.")
    project = env.get("SONAR_PROJECT_KEY", "SteamWorks")
    name = env.get("SONAR_PROJECT_NAME", "SteamWorks")
    if not project.strip() or not name.strip():
        raise SonarError("SONAR_PROJECT_KEY y SONAR_PROJECT_NAME no pueden estar vacíos.")
    return base, project, name, wait, timeout


def compilation_database(root, path):
    path = Path(path)
    if not path.is_absolute():
        path = root / path
    if not path.is_file():
        raise SonarError("Falta compile_commands.json; ejecuta make build-exts-linux primero.")
    entries = json.loads(path.read_text())
    if not isinstance(entries, list):
        raise SonarError("La base de compilación debe ser una lista.")
    selected = {}
    for entry in entries:
        directory = Path(entry["directory"])
        if not directory.is_absolute():
            raise SonarError("La base debe conservar directorios absolutos del build real.")
        source = (directory / entry["file"]).resolve()
        if not source.is_relative_to(root / "extension"):
            continue
        args = entry.get("arguments")
        if args is None:
            args = shlex.split(entry["command"])
        if not isinstance(args, list) or not all(isinstance(v, str) for v in args) or "-m32" not in args:
            raise SonarError("El análisis requiere las órdenes reales de compilación x86 (-m32).")
        selected[source] = {**entry, "file": str(source), "arguments": args}
        selected[source].pop("command", None)
    expected = {p.resolve() for p in (root / "extension").rglob("*") if p.suffix in {".c", ".cpp"}}
    if not expected or set(selected) != expected:
        raise SonarError("La base no cubre exactamente las unidades C/C++ de extension/.")
    return list(selected.values())


def executable(name):
    result = shutil.which(name)
    if not result:
        raise SonarError(f"Falta {name} en PATH; instala las herramientas explícitamente.")
    return result


def redact(text, token):
    if token:
        for secret in (token, base64.b64encode((token + ":").encode()).decode()):
            text = text.replace(secret, "[REDACTED]")
    return text


def run(action, default_database, root=ROOT):
    env = configuration(root)
    context = scanner_context(env) if action == "analyze" else None
    cppcheck = executable("cppcheck")
    scanner = executable("sonar-scanner") if context else None
    entries = compilation_database(root, env.get("SONAR_COMPILE_COMMANDS") or default_database)
    # Each run gets a private output directory. A failed run cannot reuse old XML.
    parent = root / ".build/sonar"
    parent.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix="run-", dir=parent))
    database = output / "compile_commands.json"
    database.write_text(json.dumps(entries, indent=2) + "\n")
    report = output / "cppcheck.xml"
    local_env = {k: v for k, v in env.items() if not k.startswith("SONAR_")}
    result = subprocess.run([cppcheck, f"--project={database}", "--platform=unix32",
                             "--enable=warning,style,performance,portability", "--xml", "--xml-version=2"],
                            cwd=root, env=local_env, capture_output=True, text=True)
    if result.returncode:
        raise SonarError("Cppcheck falló; no se ejecutó SonarScanner.")
    try:
        xml = ET.fromstring(result.stderr)
        if xml.tag != "results" or xml.find("errors") is None:
            raise ValueError()
    except (ET.ParseError, ValueError):
        raise SonarError("Cppcheck no produjo un informe XML válido; scanner cancelado.") from None
    report.write_text(result.stderr)
    print(f"[sonar] Informe Cppcheck: {report.relative_to(root)}", flush=True)
    if context is None:
        return
    base, project, name, wait, timeout = context
    # The token travels only in the scanner environment, never in argv/properties.
    result = subprocess.run([scanner, f"-Dsonar.host.url={base}", f"-Dsonar.projectKey={project}",
                             f"-Dsonar.projectName={name}",
                             f"-Dsonar.cxx.jsonCompilationDatabase={database}",
                             f"-Dsonar.cxx.cppcheck.reportPaths={report}",
                             f"-Dsonar.working.directory={output / 'scanner'}",
                             f"-Dsonar.qualitygate.wait={wait}", f"-Dsonar.qualitygate.timeout={timeout}"],
                            cwd=root, env={**env, "SONAR_HOST_URL": base}, capture_output=True, text=True)
    print(redact(result.stdout + result.stderr, env["SONAR_TOKEN"]))
    if result.returncode:
        raise SonarError("SonarScanner o el quality gate falló; consulta la salida saneada.")
    print("[sonar] SonarScanner completado" + (" y quality gate aprobado." if wait == "true" else "; gate no esperado."))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("analyze", "cppcheck"))
    parser.add_argument("--compile-commands", required=True)
    args = parser.parse_args()
    try:
        run(args.action, args.compile_commands)
    except (SonarError, OSError, ValueError, KeyError, TypeError):
        # Do not expose configuration values or subprocess output in a traceback.
        error = sys.exc_info()[1]
        print(str(error) if isinstance(error, SonarError) else "[sonar] Error local de configuración o ejecución.", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
