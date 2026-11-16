from pathlib import Path
import subprocess,time,json,datetime
lab=Path('/data/serverfiles/left4dead2/addons/sourcemod_acceptance');console=Path('/data/log/console/l4d2server-console.log')
def active():
 out=[]
 for p in Path('/proc').glob('[0-9]*/comm'):
  try:
   if p.read_text().strip()=='srcds_linux':out.append(int(p.parent.name))
  except OSError:pass
 return out
pids=active();assert len(pids)==1
subprocess.run(['tmux','-L','l4d2server-9a781358','send-keys','-t','l4d2server','quit','Enter'],check=True)
for _ in range(250):
 if pids[0] not in active():break
 time.sleep(.1)
else:raise SystemExit('Controlled quit timeout')
lines=console.read_text(errors='replace').splitlines()
r=subprocess.run(['/usr/local/libexec/l4d2/login-shell','-c','cd /app && ./l4d2server stop'],capture_output=True,text=True,timeout=35);assert r.returncode==0 and not active()
print(json.dumps({'timestamp_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'quit_completed':True,'lab_stopped':True,'stopped_pid':pids[0],'trace':[s for s in lines if s.startswith('[SW-RESTORE]')],'lifecycle':[s for p in (lab/'logs').glob('L*.log') for s in p.read_text(errors='replace').splitlines() if '[STEAMWORKS]' in s],'error_log_sizes':{p.name:p.stat().st_size for p in (lab/'logs').glob('errors_*.log')},'crash_markers':[k for k in ['Segmentation fault','core dumped','Alarm clock','Host_Error'] if any(k in s for s in lines)]},indent=2))
