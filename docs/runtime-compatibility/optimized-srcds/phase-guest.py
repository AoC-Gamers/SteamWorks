from pathlib import Path
import json,subprocess,sys,time,datetime
cfg=json.load(sys.stdin)
console=Path('/data/log/console/l4d2server-console.log'); lab=Path('/data/serverfiles/left4dead2/addons/sourcemod_acceptance')
before=len(console.read_text(errors='replace').splitlines())
def send(command):subprocess.run(['tmux','-L','l4d2server-9a781358','send-keys','-t','l4d2server',command,'Enter'],check=True)
for command in cfg['commands']:send(command);time.sleep(cfg.get('command_gap', .25))
time.sleep(cfg.get('wait_seconds',7))
send('sm_sw_accept status');time.sleep(.5)
lines=console.read_text(errors='replace').splitlines()
trace=[s for s in lines[before:] if s.startswith(('[SW-ACCEPT]','[SW-PENDING]'))]
print(json.dumps({'timestamp_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'phase':cfg['phase'],'commands':cfg['commands'],'trace':trace,'engine_connections':sum('Connection to Steam servers successful.' in s for s in lines[before:]),'debug':(lab/'logs/steamworks/debug.log').read_text().splitlines(),'steamworks_lifecycle':[s for p in (lab/'logs').glob('L*.log') for s in p.read_text(errors='replace').splitlines() if '[STEAMWORKS]' in s],'error_log_sizes':{p.name:p.stat().st_size for p in (lab/'logs').glob('errors_*.log')},'crash_markers':[k for k in ['Segmentation fault','core dumped','Alarm clock','Host_Error'] if any(k in s for s in lines[before:])]},indent=2))
