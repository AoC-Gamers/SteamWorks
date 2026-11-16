from pathlib import Path
import subprocess,json,time
w=Path('.build/review12-srcds')
code='''from pathlib import Path
import json,time
console=Path('/data/log/console/l4d2server-console.log');lab=Path('/data/serverfiles/left4dead2/addons/sourcemod_acceptance')
for _ in range(120):
 lines=console.read_text(errors='replace').splitlines()
 if any('[SW-BUFFER] ready' in s for s in lines) and any('Connection to Steam servers successful.' in s for s in lines):break
 time.sleep(.25)
else:raise SystemExit('Laboratory readiness timeout')
pids=[]
for p in Path('/proc').glob('[0-9]*'):
 try:
  if (p/'comm').read_text().strip()!='srcds_linux':continue
  args=(p/'cmdline').read_bytes().split(b'\\0')
  assert b'addons/sourcemod_acceptance' in args and b'27015' in args
  pids.append(int(p.name))
 except OSError:pass
assert len(pids)==1
errors={p.name:p.stat().st_size for p in (lab/'logs').glob('errors_*.log') if p.stat().st_size}
assert not errors,errors
print(json.dumps({'ready':True,'PID':pids[0],'Steam_connected':True,'isolated_selector_verified':True,'port':27015,'raw_args_exported':False}))
'''
p=subprocess.run(['docker','exec','--user','linuxgsm','competitive_dev','python3','-c',code],capture_output=True,text=True,timeout=40);assert p.returncode==0,p.stderr;(w/'final-ready.json').write_text(p.stdout);print(p.stdout)
