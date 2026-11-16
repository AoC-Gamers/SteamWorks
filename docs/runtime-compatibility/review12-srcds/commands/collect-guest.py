from pathlib import Path
import os,json,hashlib,datetime
lab=Path('/data/serverfiles/left4dead2/addons/sourcemod_acceptance')
selected=[]
for p in Path('/proc').glob('[0-9]*'):
 try:
  if (p/'comm').read_text().strip()=='srcds_linux' and b'addons/sourcemod_acceptance' in (p/'cmdline').read_bytes().split(b'\0'):selected.append(p)
 except OSError:pass
if len(selected)!=1:raise SystemExit('Expected exactly one isolated laboratory process')
pid=selected[0];records={}
for line in (pid/'maps').read_text().splitlines():
 parts=line.split(maxsplit=5)
 if len(parts)<6:continue
 path=Path(parts[5])
 if path.name not in ['engine_srv.so','sourcetvsupport.ext.2.l4d2.so','sourcetvmanager.ext.2.l4d2.so','server_srv.so','libtier0_srv.so','crashhandler.so','libsteam_api.so','steamclient.so','steamworks.ext.so','steamworks_buffer_fixture.ext.so','sourcemod.2.l4d2.so','sourcemod.logic.so','sourcepawn.jit.x86.so','metamod.2.l4d2.so']:continue
 if str(path) in records:continue
 stat=path.stat();major,minor=(int(x,16) for x in parts[3].split(':'))
 if stat.st_ino!=int(parts[4]) or stat.st_dev!=os.makedev(major,minor):raise SystemExit('Mapped file identity differs')
 records[str(path)]={'path':str(path),'file_name':path.name,'size_bytes':stat.st_size,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'device_major':major,'device_minor':minor,'inode':stat.st_ino,'loaded_module_confirmed':True}
print(json.dumps({'timestamp_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'container':'competitive_dev','lab_pid':int(pid.name),'process_start_ticks':int((pid/'stat').read_text().rsplit(')',1)[1].split()[19]),'executable':os.readlink(pid/'exe'),'modules':list(records.values()),'cmdline_and_environment_omitted':True},indent=2))
