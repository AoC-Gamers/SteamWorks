from pathlib import Path
import json,subprocess,sys,time,datetime,re
cfg=json.load(sys.stdin);console=Path('/data/log/console/l4d2server-console.log');lab=Path('/data/serverfiles/left4dead2/addons/sourcemod_acceptance')
before=len(console.read_text(errors='replace').splitlines())
def send(command):subprocess.run(['tmux','-L','l4d2server-9a781358','send-keys','-t','l4d2server',command,'Enter'],check=True)
for command in cfg['commands']:send(command);time.sleep(.2)
time.sleep(cfg.get('wait_seconds',.5))
name=cfg['phase'];assert re.fullmatch('[a-z0-9-]+',name)
dump=lab/'logs'/('owner-handles-'+name+'.txt');assert not dump.exists()
dump_start=len(console.read_text(errors='replace').splitlines())
send('sm_dump_handles addons/sourcemod_acceptance/logs/'+dump.name)
send('sm_sw_restore_check')
for _ in range(80):
 fresh=console.read_text(errors='replace').splitlines()[dump_start:]
 if dump.exists() and any(s.startswith('[SW-RESTORE] status ') for s in fresh):break
 time.sleep(.05)
else:raise SystemExit('Handle dump completion unavailable')
owned=[]
for line in dump.read_text(errors='replace').splitlines():
 parts=[s.strip() for s in line.split('\t')]
 if len(parts)>=4 and parts[0].startswith('0x') and parts[1]=='steamworks_pending_owner.smx'[:20] and parts[2]=='HTTPHandle':owned.append({'handle':parts[0],'type':parts[2],'approx_bytes':int(parts[3])})
dump.unlink()
start=len(console.read_text(errors='replace').splitlines());send('sm plugins list');time.sleep(.3)
lines=console.read_text(errors='replace').splitlines();plugins=[s for s in lines[start:] if re.match(r'^\s*\d+\s+(?:<\w+>\s+)?"',s)]
print(json.dumps({'timestamp_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'phase':name,'commands':cfg['commands'],'trace':[s for s in lines[before:] if s.startswith(('[SW-OWNER]','[SW-RESTORE]'))],'all_owner_trace':[s for s in lines if s.startswith('[SW-OWNER]')],'owner_HTTP_handles':owned,'owner_HTTP_handle_count':len(owned),'owner_plugin_rows':[s for s in plugins if '"SteamWorks Pending Owner"' in s],'plugin_rows':len(plugins),'plugin_error_rows':[s for s in plugins if '<Error>' in s or '<Failed>' in s],'error_log_sizes':{p.name:p.stat().st_size for p in (lab/'logs').glob('errors_*.log')},'crash_markers':[k for k in ['Segmentation fault','core dumped','Alarm clock','Host_Error'] if any(k in s for s in lines)],'raw_handle_dump_removed':not dump.exists()},indent=2))
