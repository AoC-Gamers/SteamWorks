from pathlib import Path
import json,subprocess,time,datetime,hashlib,re
cfg=json.load(__import__('sys').stdin);console=Path('/data/log/console/l4d2server-console.log');lab=Path('/data/serverfiles/left4dead2/addons/sourcemod_acceptance')
for _ in range(160):
 lines=console.read_text(errors='replace').splitlines() if console.exists() else []
 if any('[SW-RESTORE] plugin_ready' in s for s in lines) and any('Connection to Steam servers successful.' in s for s in lines):break
 time.sleep(.25)
else:raise SystemExit('Isolated laboratory not ready')
def send(command):subprocess.run(['tmux','-L','l4d2server-9a781358','send-keys','-t','l4d2server',command,'Enter'],check=True)
send('sm_sw_restore_check http')
for _ in range(160):
 lines=console.read_text(errors='replace').splitlines()
 if any('[SW-RESTORE] request_closed' in s for s in lines):break
 time.sleep(.1)
else:raise SystemExit('HTTP smoke did not complete and close')
send('sm_sw_restore_check');time.sleep(.4)
start=len(console.read_text(errors='replace').splitlines());send('sm plugins list');time.sleep(.4)
lines=console.read_text(errors='replace').splitlines();plugin_lines=[s for s in lines[start:] if re.match(r'^\s*\d+\s+(?:<\w+>\s+)?"',s)]
trace=[s for s in lines if s.startswith('[SW-RESTORE]')]
assert any('complete pass=1 failure=0 success=1 status=200 body=1 context=1' in s for s in trace)
assert any('status loaded=1 connected=1' in s and 'completed=1 failed=0' in s for s in trace)
errors={p.name:p.stat().st_size for p in (lab/'logs').glob('errors_*.log')};assert not errors,errors
print(json.dumps({'timestamp_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'phase':cfg['phase'],'trace':trace,'plugin_list':plugin_lines,'error_log_sizes':errors,'Steam_connected':True,'lab_file_sha256':{n:hashlib.sha256((lab/n).read_bytes()).hexdigest() for n in ['extensions/steamworks.ext.so','scripting/include/steamworks.inc','configs/core.cfg','configs/restore-sentinel.cfg']},'crash_markers':[k for k in ['Segmentation fault','core dumped','Alarm clock','Host_Error'] if any(k in s for s in lines)]},indent=2))
