from pathlib import Path
import subprocess,json,hashlib
w=Path('.build/review12-srcds')
p=subprocess.run(['docker','exec','--user','linuxgsm','competitive_dev','python3','-c',(w/'stop-guest.py').read_text()],capture_output=True,text=True,timeout=60);assert p.returncode==0,p.stderr;(w/'shutdown.json').write_text(p.stdout);j=json.loads(p.stdout);print(json.dumps({'controlled_quit':j['quit_completed'],'crash_markers':j['crash_markers'],'error_logs':j['error_log_sizes']}),flush=True)
restore='''from pathlib import Path
import hashlib,json
lab=Path('/data/serverfiles/left4dead2/addons/sourcemod_acceptance');cfg=Path('/data/config-lgsm/l4d2server/l4d2server.cfg')
for p in Path('/proc').glob('[0-9]*/comm'):
 try:assert p.read_text().strip()!='srcds_linux'
 except OSError:pass
meta=json.loads((lab/'recovery/baseline.json').read_text());assert hashlib.sha256(cfg.read_bytes()).hexdigest()==meta['selected_sha256']
original=(lab/'recovery/original.cfg').read_bytes();assert hashlib.sha256(original).hexdigest()==meta['original_files_sha256'][str(cfg)]
cfg.write_bytes(original)
print(json.dumps({'original_selector_restored':True,'cfg_sha256':hashlib.sha256(cfg.read_bytes()).hexdigest()}))
'''
p=subprocess.run(['docker','exec','--user','linuxgsm','competitive_dev','python3','-c',restore],capture_output=True,text=True);assert p.returncode==0,p.stderr;(w/'restored-selector.json').write_text(p.stdout)
with (w/'start-original.log').open('w') as log:p=subprocess.run(['docker','exec','--user','linuxgsm','competitive_dev','/usr/local/libexec/l4d2/login-shell','-c','cd /app && ./l4d2server start'],stdout=log,stderr=subprocess.STDOUT,timeout=50)
assert p.returncode==0,'Original DEV LinuxGSM start failed; inspect private log'
# This checked cleanup verifies all protected files and that the lab DSO is absent.
p=subprocess.run(['docker','exec','--user','linuxgsm','competitive_dev','python3','-c',Path('.build/http-stream-srcds/cleanup-guest.py').read_text()],capture_output=True,text=True,timeout=35);assert p.returncode==0,p.stderr;(w/'cleanup.json').write_text(p.stdout)
s=json.loads((w/'session.json').read_text());s.update(phase='finished_original_DEV_restored',original_DEV_restored=True,lab_removed=True,production_touched=False);(w/'session.json').write_text(json.dumps(s,indent=2));print('ORIGINAL_DEV_RESTORED_AND_HASHES_VERIFIED_LAB_REMOVED',flush=True)
