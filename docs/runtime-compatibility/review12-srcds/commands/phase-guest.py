from pathlib import Path
import json,subprocess,sys,time,hashlib,datetime
cfg=json.load(sys.stdin);lab=Path('/data/serverfiles/left4dead2/addons/sourcemod_acceptance');console=Path('/data/log/console/l4d2server-console.log');before=len(console.read_text(errors='replace').splitlines());logbefore={p.name:len(p.read_text(errors='replace').splitlines()) for p in (lab/'logs').glob('L*.log')}
for c in cfg['commands']:
 subprocess.run(['tmux','-L','l4d2server-9a781358','send-keys','-t','l4d2server',c,'Enter'],check=True);time.sleep(.3)
time.sleep(cfg.get('wait_seconds',1));lines=console.read_text(errors='replace').splitlines();trace=[]
for p in sorted((lab/'logs').glob('L*.log')):
 for s in p.read_text(errors='replace').splitlines()[logbefore.get(p.name,0):]:
  if '[SW-BUFFER]' in s:trace.append(s[s.index('[SW-BUFFER]'):])
pids=[]
for p in Path('/proc').glob('[0-9]*/comm'):
 try:
  if p.read_text().strip()=='srcds_linux':pids.append({'pid':int(p.parent.name),'start_ticks':int((p.parent/'stat').read_text().rsplit(')',1)[1].split()[19])})
 except OSError:pass
errors={p.name:{'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in (lab/'logs').glob('errors_*.log') if p.stat().st_size}
print(json.dumps({'timestamp_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'phase':cfg['phase'],'commands_redacted_for_binding':cfg['phase'].startswith('GC'),'driver_trace':trace,'console_trace':[s for s in lines[before:] if s.startswith('[SW-BUFFER]')],'processes':pids,'error_log_metadata':errors,'Steam_connected':any('Connection to Steam servers successful.' in s for s in lines),'crash_markers':[k for k in ['Segmentation fault','core dumped','Alarm clock','Host_Error'] if any(k in s for s in lines)],'raw_console_and_logs_exported':False},indent=2))
