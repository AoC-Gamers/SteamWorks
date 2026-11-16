from pathlib import Path
import importlib.util,json,signal,threading
from http.server import ThreadingHTTPServer
spec=importlib.util.spec_from_file_location('fixture',Path('tests/runtime/http_fixture.py').resolve());fixture=importlib.util.module_from_spec(spec);spec.loader.exec_module(fixture)
server=ThreadingHTTPServer(('127.0.0.1',27880),fixture.Handler);stop=threading.Event()
for sig in [signal.SIGINT,signal.SIGTERM]:signal.signal(sig,lambda *_:stop.set())
thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start();print('LOOPBACK_FIXTURE_READY',flush=True);stop.wait();server.shutdown();server.server_close();thread.join()
Path('.build/review12-srcds/fixture-counts.json').write_text(json.dumps(fixture.counts,indent=2)+'\n')
