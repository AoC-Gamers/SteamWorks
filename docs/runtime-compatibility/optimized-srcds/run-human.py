from pathlib import Path
import json,subprocess
w=Path('.build/optimized-srcds')
def guest(script,target,timeout=50):
 p=subprocess.run(['docker','exec','--user','linuxgsm','competitive_dev','python3','-c',Path(script).read_text()],capture_output=True,text=True,timeout=timeout);assert p.returncode==0,p.stderr;(w/target).write_text(p.stdout);return json.loads(p.stdout)
def check(script,source,target):
 p=subprocess.run(['python3',script,str(w/source)],capture_output=True,text=True);(w/target).write_text(p.stdout);assert p.returncode==0,p.stdout+p.stderr;return json.loads(p.stdout)
x=guest('.build/p5-r3-auth-srcds/watch-guest.py','runtime-final.json')
assert x['human_present'],'No authorized human detected; keep lab available for pending connection'
check('.build/p5-r6-auth-srcds/check-auth.py','runtime-final.json','checks-auth.json');print('AUTH_IDENTITY_INTEGER_STATS_PASSED',flush=True)
for i in [1,2]:
 guest('.build/p5-r6-auth-srcds/stats-round-guest.py','stats-round-'+str(i)+'.json',55)
 check('.build/p5-r6-auth-srcds/check-stats.py','stats-round-'+str(i)+'.json','checks-stats-'+str(i)+'.json');print('FLOAT_MISSING_KEYS_ROUND_'+str(i)+'_PASSED',flush=True)
a=json.loads((w/'stats-round-1.json').read_text());b=json.loads((w/'stats-round-2.json').read_text())
keys=lambda x:sorted(s.split('key=')[1].split()[0] for s in x['trace'] if '[SW-STATS] float key=' in s)
assert keys(a)==keys(b) and len(keys(a))>0
p=subprocess.run(['docker','exec','--user','linuxgsm','competitive_dev','python3','-c',(w/'collect-guest.py').read_text()],capture_output=True,text=True);assert p.returncode==0,p.stderr;(w/'loaded-modules-final.json').write_text(p.stdout)
start=json.loads((w/'initial-modules.json').read_text());end=json.loads(p.stdout)
assert (start['lab_pid'],start['process_start_ticks'])==(end['lab_pid'],end['process_start_ticks'])
(w/'checks-human-summary.json').write_text(json.dumps({'passed':True,'same_PID':True,'float_keys_in_both_rounds':keys(a),'successful_float_key_count':len(keys(a)),'personal_values_exported':False,'stats_writes':False},indent=2)+'\n')
print('HUMAN_ACCEPTANCE_PASSED',flush=True)
