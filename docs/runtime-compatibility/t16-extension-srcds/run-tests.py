from pathlib import Path
import subprocess,json,re,time,datetime
w=Path('.build/t16-extension-srcds');initial=json.loads((w/'cold-absent.json').read_text());identity=(initial['PID'],initial['start_ticks'])
def guest(script):
 p=subprocess.run(['docker','exec','--user','linuxgsm','competitive_dev','python3','-c',script],capture_output=True,text=True);assert p.returncode==0,p.stderr
 return p.stdout
def phase(name,commands,wait=.5):
 p=subprocess.run(['docker','exec','--user','linuxgsm','-i','competitive_dev','python3','-c',(w/'phase-guest.py').read_text()],input=json.dumps({'phase':name,'commands':commands,'wait_seconds':wait}),capture_output=True,text=True,timeout=35);assert p.returncode==0,p.stderr;(w/(name+'.json')).write_text(p.stdout);j=json.loads(p.stdout)
 assert (j['PID'],j['start_ticks'])==identity
 assert not j['error_log_sizes'] and not j['crash_markers'],(name,j['error_log_sizes'],j['crash_markers'])
 assert not any('<Error>' in s or '<Failed>' in s for s in j['plugin_list'])
 return j
def code(j,name):return next(s.split('bytes=')[1] for s in reversed(j['trace']) if 'code='+name+' ' in s)
def pipes(j):
 line=next(s for s in reversed(j['trace']) if 'pipes client=' in s);m=re.search(r'client=(\d+) server=(\d+)',line);assert m;return (int(m[1]),int(m[2]))
def library(j,present,added,removed):
 assert any('library present='+str(present)+' added='+str(added)+' removed='+str(removed) in s and 'convar='+str(present) in s for s in j['trace'])
def absent(j,added,removed):
 library(j,0,added,removed);assert j['extension_id'] is None and 'steamworks.ext.so' not in j['modules']
 assert pipes(j)==pipes(initial) and pipes(j)[1]>0
 for n in ['engine','shutdown','init']:assert code(j,n)==code(initial,n),(n,code(j,n),code(initial,n))
def loaded(j,added,removed):
 library(j,1,added,removed);assert j['extension_id'] is not None
 assert j['modules']['steamworks.ext.so']['sha256']=='839ff0751b04141031bdc6dffc48662d0804f52227f2c91015bd87c6b84cf192'
 assert pipes(j)==pipes(initial)
 assert code(j,'engine')!=code(initial,'engine') and code(j,'shutdown')!=code(initial,'shutdown')
 assert code(j,'init')==code(initial,'init') # SourceTVSupport entry untouched.
def probe(name):
 guest("from pathlib import Path;import shutil;p=Path('/data/serverfiles/left4dead2/addons/sourcemod_acceptance');shutil.copyfile(p/'recovery/steamworks_rollback_acceptance.smx',p/'plugins/steamworks_rollback_acceptance.smx')")
 j=phase(name,['sm plugins load steamworks_rollback_acceptance.smx','sm_sw_restore_check http'],3)
 assert any('complete pass=1 failure=0 success=1 status=200 body=1 context=1' in s for s in j['trace']) and any('request_closed' in s for s in j['trace'])
 return j
absent(initial,0,0);assert 'sourcetvsupport.ext.2.l4d2.so' in initial['modules']
guest("from pathlib import Path; p=Path('/data/serverfiles/left4dead2/addons/sourcemod_acceptance'); (p/'recovery/steamworks.ext.so').rename(p/'extensions/steamworks.ext.so')")
j=phase('late-load',['sm exts load steamworks','steamworks_debug 15']);loaded(j,1,0);probe('late-load-http');print('COLD_LATE_LOAD_HTTP_PASSED',flush=True)
for cycle in [1,2]:
 j=phase('remove-consumer-'+str(cycle),['sm plugins unload steamworks_rollback_acceptance.smx']);loaded(j,cycle,cycle-1)
 ext=j['extension_id'];j=phase('unloaded-'+str(cycle),['sm exts unload '+str(ext)]);absent(j,cycle,cycle);print('HOT_UNLOAD_'+str(cycle)+'_UNMAPPED_HOOKS_RESTORED_PIPES_STABLE',flush=True)
 # Stay absent across additional frames; no callback/cvar/library auto-revival.
 j=phase('absent-window-'+str(cycle),[],3);absent(j,cycle,cycle)
 j=phase('reloaded-'+str(cycle),['sm exts load steamworks','steamworks_debug 15']);loaded(j,cycle+1,cycle)
 probe('reloaded-http-'+str(cycle));print('RELOAD_'+str(cycle)+'_HTTP_PASSED',flush=True)
# Prove reloaded observers remain attached to a real new-game path.
j=phase('newgame-after-reloads',['map c2m2_fairgrounds'],12);library(j,1,3,2);assert j['modules']['steamworks.ext.so']['sha256']=='839ff0751b04141031bdc6dffc48662d0804f52227f2c91015bd87c6b84cf192'
assert any('init observed: result=1' in s for s in j['lifecycle'])
assert pipes(j)[1]>0
j=phase('post-newgame-http',['sm_sw_restore_check http'],3);assert any('complete pass=1 failure=0 success=1 status=200 body=1 context=1' in s for s in j['trace'])
assert any('completed=2 failed=0' in s for s in j['trace'])
(w/'checks.json').write_text(json.dumps({'passed':True,'PID':initial['PID'],'same_PID_start_ticks':True,'cold_start_without_SteamWorks':True,'cold_late_load':True,'hot_unload_reload_cycles':2,'no_active_consumers_or_requests_at_extension_unload':True,'forced_unload_confirmation_used':False,'module_unmapped_each_unload':True,'engine_CALL_and_shutdown_bytes_restored_exactly':True,'SourceTVSupport_init_bytes_unchanged':True,'client_and_server_pipes_unchanged_during_unload_reload':list(pipes(initial)),'absent_observation_seconds_each_cycle':3,'library_added_events':3,'library_removed_events':2,'CVar_removed_and_reregistered':True,'HTTP_smoke_rounds':4,'newgame_after_reload_passed':True,'late_load_consumer_coverage':'minimal isolated harnesses; original plugin stack quarantined in lab only'},indent=2)+'\n')
print('LATE_LOAD_UNLOAD_RELOAD_ACCEPTANCE_PASSED',flush=True)
