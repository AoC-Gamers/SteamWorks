from pathlib import Path
import subprocess,json,sys
r=Path('.build/p5-r19-rollback-srcds');phase=sys.argv[1]
x=subprocess.run(['docker','exec','-i','--user','linuxgsm','competitive_dev','python3','-c',(r/'smoke-guest.py').read_text()],input=json.dumps({'phase':phase}),capture_output=True,text=True,timeout=45);assert x.returncode==0,x.stderr;(r/(phase+'-runtime.json')).write_text(x.stdout);d=json.loads(x.stdout);assert not d['crash_markers']
collect=(r/'collect-guest.py').read_text();(r/'collect-guest.py').write_text(collect)
x=subprocess.run(['docker','exec','--user','linuxgsm','competitive_dev','python3','-c',collect],capture_output=True,text=True);assert x.returncode==0,x.stderr;(r/(phase+'-modules.json')).write_text(x.stdout);modules=json.loads(x.stdout)
manifest=json.loads(Path('.build/p5-r19/review/manifest.json').read_text());expected=manifest['candidate_files']['addons/sourcemod/extensions/steamworks.ext.so'] if phase=='candidate' else manifest['rollback_files']['extensions/steamworks.ext.so']
assert any(m['file_name']=='steamworks.ext.so' and m['sha256']==expected for m in modules['modules'])
host=[]
for line in subprocess.check_output(['docker','top','competitive_dev','-eo','pid,comm'],text=True).splitlines()[1:]:
 a=line.split()
 if len(a)==2 and a[1]=='srcds_linux':host.append(int(a[0]))
assert len(host)==1
(r/(phase+'-process-identity.json')).write_text(json.dumps({'role':phase,'guest_pid':modules['lab_pid'],'host_pid':host[0],'host_PID_source':'docker top competitive_dev -eo pid,comm; same live phase','argv_and_environment_omitted':True},indent=2)+'\n')
print(json.dumps({'phase':phase,'pid':modules['lab_pid'],'mapped_extension_sha256':expected,'trace':d['trace'],'plugin_rows':len(d['plugin_list'])},indent=2))
