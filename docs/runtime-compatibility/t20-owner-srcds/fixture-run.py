from pathlib import Path
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
import json,threading,signal,time
root=Path('.build/t20-owner-srcds');lock=threading.Lock();events={};stop=threading.Event()
def event(path,key):
 with lock:
  events.setdefault(path,{})[key]=time.time();(root/'fixture-events.json').write_text(json.dumps(events,indent=2)+'\n')
class Handler(BaseHTTPRequestHandler):
 def log_message(self,*args):pass
 def do_GET(self):
  path=self.path
  if not(path=='/text' or path.startswith('/text/') or path.startswith('/hold/')):
   self.send_error(404);return
  event(path,'entered')
  if path.startswith('/hold/'):
   sequence=int(path.rsplit('/',1)[1]);deadline=time.monotonic()+100
   while not(root/('release-'+str(sequence))).exists() and not stop.is_set():
    if time.monotonic()>deadline:raise RuntimeError('Unreleased laboratory hold')
    time.sleep(.03)
   if stop.is_set():return
   event(path,'released')
  try:
   self.send_response(200);self.send_header('Content-Length','16');self.end_headers();self.wfile.write(b'steamworks-local');self.wfile.flush();event(path,'response_written')
  except (BrokenPipeError,ConnectionResetError):event(path,'peer_closed')
server=ThreadingHTTPServer(('127.0.0.1',27880),Handler);server.daemon_threads=True
for sig in [signal.SIGTERM,signal.SIGINT]:signal.signal(sig,lambda *_:stop.set())
t=threading.Thread(target=server.serve_forever,daemon=True);t.start();print('OWNER_FIXTURE_READY',flush=True);stop.wait();server.shutdown();server.server_close();t.join()
(root/'fixture-stopped.json').write_text(json.dumps({'stopped':True,'events':events},indent=2)+'\n')
