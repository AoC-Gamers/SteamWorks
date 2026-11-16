from pathlib import Path
import subprocess,json,sys
w=Path('.build/review12-srcds');cfg=json.loads((w/'getter-profile.json').read_text());cfg.update(phase=sys.argv[1],local='local' in sys.argv[1],phase_code=(w/'phase-guest.py').read_text())
p=subprocess.run(['docker','exec','--user','linuxgsm','-i','competitive_dev','python3','-c',(w/'bind-guest.py').read_text()],input=json.dumps(cfg),capture_output=True,text=True,timeout=30);assert p.returncode==0,p.stderr;(w/(cfg['phase']+'.json')).write_text(p.stdout);j=json.loads(p.stdout);print(json.dumps(j,indent=2))
