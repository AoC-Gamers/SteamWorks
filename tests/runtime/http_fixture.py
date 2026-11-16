"""Loopback-only HTTP/invalid-TLS fixtures; never logs request headers or bodies."""
import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import signal
import ssl
import threading
import time

counts = {}
lock = threading.Lock()


class Handler(BaseHTTPRequestHandler):
    def log_message(self, *args):
        pass

    def do_GET(self):
        with lock:
            counts[self.path] = counts.get(self.path, 0) + 1
        if self.path == "/delay":
            time.sleep(3)
        body = {"/text": b"steamworks-local", "/binary": b"A\0B\xff\0Z",
                "/stream": b"A\0B\xff\0Z", "/empty": b"",
                "/error": b"fixture-error", "/delay": b"delayed"}.get(self.path, b"missing")
        try:
            self.send_response(503 if self.path == "/error" else 200)
            self.send_header("Content-Length", str(len(body)))
            self.send_header("X-Lab", "local-fixture")
            self.end_headers()
            if self.path == "/stream":
                self.wfile.write(body[:3])
                self.wfile.flush()
                time.sleep(0.15)
                self.wfile.write(body[3:])
            else:
                self.wfile.write(body)
        except (BrokenPipeError, ConnectionResetError, ssl.SSLError):
            pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cert", required=True)
    parser.add_argument("--key", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    servers = [ThreadingHTTPServer(("127.0.0.1", 27880), Handler),
               ThreadingHTTPServer(("127.0.0.1", 27881), Handler)]
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(args.cert, args.key)
    servers[1].socket = context.wrap_socket(servers[1].socket, server_side=True)
    stop = threading.Event()
    for sig in [signal.SIGTERM, signal.SIGINT]:
        signal.signal(sig, lambda *_: stop.set())
    threads = [threading.Thread(target=s.serve_forever, daemon=True) for s in servers]
    for thread in threads:
        thread.start()
    print("LOCAL_FIXTURES_READY", flush=True)
    stop.wait()
    for server in servers:
        server.shutdown()
        server.server_close()
    Path(args.output).write_text(json.dumps(counts, indent=2) + "\n")


if __name__ == "__main__":
    main()
