from pathlib import Path
import json,subprocess,sys,time,datetime,re,hashlib,os
cfg=json.load(sys.stdin);lab=Path('/data/serverfiles/left4dead2/addons/sourcemod_acceptance');console=Path('/data/log/console/l4d2server-console.log');before=len(console.read_text(errors='replace').splitlines())
def send(c):subprocess.run(['tmux','-L','l4d2server-9a781358','send-keys','-t','l4d2server',c,'Enter'],check=True)
for c in cfg['commands']:send(c);time.sleep(.2)
time.sleep(cfg.get('wait_seconds',.5))
start=len(console.read_text(errors='replace').splitlines());send('sm exts list');time.sleep(.3);ext_lines=console.read_text(errors='replace').splitlines()[start:]
start=len(console.read_text(errors='replace').splitlines());send('sm plugins list');time.sleep(.3);plugins=console.read_text(errors='replace').splitlines()[start:]
if any('\"SteamWorks Rollback Acceptance\"' in s for s in plugins):
 send('sm_sw_restore_check');time.sleep(.15)
send('sm_sw_extension_observe');time.sleep(.3)
lines=console.read_text(errors='replace').splitlines();trace=[s for s in lines[before:] if s.startswith(('[SW-EXT]','[SW-RESTORE]','[SW-LIFETIME]'))]
pids=[]
for p in Path('/proc').glob('[0-9]*'):
 try:
  if (p/'comm').read_text().strip()=='srcds_linux':pids.append(p)
 except OSError:pass
assert len(pids)==1;pid=pids[0];maps=(pid/'maps').read_text().splitlines()
records={};wanted=['steamworks.ext.so','sourcetvsupport.ext.2.l4d2.so','libsteam_api.so','engine_srv.so']
for line in maps:
 p=line.split(maxsplit=5)
 if len(p)<6 or Path(p[5]).name not in wanted:continue
 path=Path(p[5]);st=path.stat();assert st.st_ino==int(p[4]) and st.st_dev==os.makedev(*[int(x,16) for x in p[3].split(':')])
 records[path.name]={'path':str(path),'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'inode':st.st_ino}
ids=[int(m[1]) for s in ext_lines if (m:=re.search(r'^\s*\[(\d+)\]\s+SteamWorks\b',s))]
print(json.dumps({'timestamp_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'phase':cfg['phase'],'commands':cfg['commands'],'PID':int(pid.name),'start_ticks':int((pid/'stat').read_text().rsplit(')',1)[1].split()[19]),'trace':trace,'all_trace':[s for s in lines if s.startswith(('[SW-EXT]','[SW-RESTORE]'))],'extension_id':ids[0] if len(ids)==1 else None,'extension_list':[s for s in ext_lines if re.match(r'^\s*\[',s)],'plugin_list':[s for s in plugins if re.match(r'^\s*\d+\s+(?:<\w+>\s+)?"',s)],'modules':records,'Steam_connected_in_console':any('Connection to Steam servers successful.' in s for s in lines),'debug':(lab/'logs/steamworks/debug.log').read_text().splitlines() if (lab/'logs/steamworks/debug.log').exists() else [],'lifecycle':[s for p in (lab/'logs').glob('L*.log') for s in p.read_text(errors='replace').splitlines() if '[STEAMWORKS]' in s],'error_log_sizes':{p.name:p.stat().st_size for p in (lab/'logs').glob('errors_*.log')},'crash_markers':[k for k in ['Segmentation fault','core dumped','Alarm clock','Host_Error'] if any(k in s for s in lines)]},indent=2))
