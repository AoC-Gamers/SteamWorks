from pathlib import Path
import subprocess,json,datetime
r=Path.cwd();w=r/'.build/p5-r19-rollback-srcds'
assert json.loads((w/'session.json').read_text())['phase']=='baseline_restore_start_and_HTTP_passed_awaiting_final_quit_and_DEV_restore'
def guest(script,target,timeout=75):
 p=subprocess.run(['docker','exec','--user','linuxgsm','competitive_dev','python3','-c',script],capture_output=True,text=True,timeout=timeout);assert p.returncode==0,p.stderr;(w/target).write_text(p.stdout);return json.loads(p.stdout)
stop=(r/'.build/p5-r5-rollback-srcds/stop-guest.py').read_text().replace('range(100)','range(250)')
x=guest(stop,'rollback-shutdown.json');print(json.dumps({'baseline_quit_completed':x['quit_completed'],'crash_markers':x['crash_markers'],'SM_error_log_sizes':x['error_log_sizes']}),flush=True)
restore='''from pathlib import Path
import hashlib,json
lab=Path('/data/serverfiles/left4dead2/addons/sourcemod_acceptance');cfg=Path('/data/config-lgsm/l4d2server/l4d2server.cfg')
assert not any(p.read_text().strip()=='srcds_linux' for p in Path('/proc').glob('[0-9]*/comm'))
meta=json.loads((lab/'recovery/baseline.json').read_text());assert hashlib.sha256(cfg.read_bytes()).hexdigest()==meta['selected_sha256']
original=(lab/'recovery/original.cfg').read_bytes();assert hashlib.sha256(original).hexdigest()==meta['original_files_sha256'][str(cfg)]
cfg.write_bytes(original)
print(json.dumps({'original_selector_restored':True,'cfg_sha256':hashlib.sha256(cfg.read_bytes()).hexdigest()}))
'''
guest(restore,'restored-selector.json')
with (w/'start-original.log').open('w') as log:p=subprocess.run(['docker','exec','--user','linuxgsm','competitive_dev','/usr/local/libexec/l4d2/login-shell','-c','cd /app && ./l4d2server start'],stdout=log,stderr=subprocess.STDOUT,timeout=50)
assert p.returncode==0,'LinuxGSM original start failed; raw log remains private'
guest((w/'cleanup-guest.py').read_text(),'cleanup.json',60)
s=json.loads((w/'session.json').read_text());s.update(updated_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),phase='runtime_finished_original_DEV_restored',lab_removed=True,original_DEV_restored=True);(w/'session.json').write_text(json.dumps(s,indent=2)+'\n')
print('ORIGINAL_DEV_RESTORED_STEAM_CONNECTED_LAB_REMOVED',flush=True)
