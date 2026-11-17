"""Run the disposable x86 provider probe with a loopback-only HTTP fixture.

Invoke inside a container with --network none and a temporary HOME/workdir.
This diagnostic does not launch SRCDS or load an extension.
"""
import argparse
from http.server import ThreadingHTTPServer
import importlib.util
from pathlib import Path
import subprocess
import threading


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("module", type=Path)
    args = parser.parse_args()
    fixture = Path(__file__).resolve().parents[1] / "runtime/http_fixture.py"
    spec = importlib.util.spec_from_file_location("http_fixture", fixture)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    server = ThreadingHTTPServer(("127.0.0.1", 27880), module.Handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        return subprocess.run([str(args.executable.resolve()), str(args.module.resolve())],
                              timeout=20, check=False).returncode
    finally:
        server.shutdown()
        server.server_close()
        thread.join()


if __name__ == "__main__":
    raise SystemExit(main())
