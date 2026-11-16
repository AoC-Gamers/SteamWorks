from pathlib import Path
import subprocess,json,datetime
r=Path.cwd();w=r/'.build/p5-r19-rollback-srcds'
def state(phase):
 (w/'session.json').write_text(json.dumps({'updated_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'phase':phase,'container':'competitive_dev','port':27015,'lab_selector':'addons/sourcemod_acceptance','private_recovery':'/data/serverfiles/left4dead2/addons/sourcemod_acceptance/recovery','restore_original_DEV_after_acceptance':True,'committed':False,'production_touched':False},indent=2)+'\n')
def guest(source,target,stdin=None,timeout=75):
 p=subprocess.run(['docker','exec','--user','linuxgsm','-i','competitive_dev','python3','-c',source],input=stdin,capture_output=True,text=True,timeout=timeout);assert p.returncode==0,p.stderr;(w/target).write_text(p.stdout);return json.loads(p.stdout)
def start(phase):
 with (w/('start-'+phase+'.log')).open('w') as log:p=subprocess.run(['docker','exec','--user','linuxgsm','competitive_dev','/usr/local/libexec/l4d2/login-shell','-c','cd /app && ./l4d2server start'],stdout=log,stderr=subprocess.STDOUT,timeout=50)
 assert p.returncode==0,'LinuxGSM start failed; raw log stays private'
def smoke(phase):
 p=subprocess.run(['python3',str(w/'smoke-phase.py'),phase],capture_output=True,text=True,timeout=65);(w/(phase+'-runner.log')).write_text(p.stdout+p.stderr);assert p.returncode==0,p.stderr
 x=json.loads((w/(phase+'-runtime.json')).read_text());m=json.loads((w/(phase+'-modules.json')).read_text())
 assert len(x['plugin_list'])==1 and not any('<Error>' in s or '<Failed>' in s for s in x['plugin_list'])
 print(json.dumps({'phase':phase,'PID':m['lab_pid'],'HTTP_context_body_callback_passed':True,'plugin_rows':len(x['plugin_list'])}),flush=True)
state('preparing_isolated_actual_ZIP_install')
setup=guest((w/'setup-guest.py').read_text(),'setup.json',(w/'payload.json').read_text());assert setup['candidate_installation']['installed_directly_from_guest_ZIP']
state('candidate_ZIP_installed_starting');start('candidate');smoke('candidate')
stop=(r/'.build/p5-r5-rollback-srcds/stop-guest.py').read_text().replace('range(100)','range(250)')
x=guest(stop,'candidate-shutdown.json');assert not x['crash_markers'] and not x['error_log_sizes'];print('CANDIDATE_QUIT_CLEAN',flush=True)
state('candidate_stopped_applying_actual_rollback_ZIP')
restore=guest((w/'apply-rollback-guest.py').read_text(),'applied-rollback.json');assert restore['restored_directly_from_guest_ZIP'] and restore['selector_unchanged']
state('rollback_applied_starting_baseline');start('rollback');smoke('rollback')
a=json.loads((w/'candidate-runtime.json').read_text());b=json.loads((w/'rollback-runtime.json').read_text());assert a['plugin_list']==b['plugin_list']
a=json.loads((w/'candidate-modules.json').read_text());b=json.loads((w/'rollback-modules.json').read_text());assert a['lab_pid']!=b['lab_pid']
state('baseline_restore_start_and_HTTP_passed_awaiting_final_quit_and_DEV_restore')
print('T26_AND_T35_SCOPED_RUNTIME_CHECKS_PASSED',flush=True)
