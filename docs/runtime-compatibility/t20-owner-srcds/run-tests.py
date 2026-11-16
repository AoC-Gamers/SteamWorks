from pathlib import Path
import json,subprocess,time,datetime
w=Path('.build/t20-owner-srcds')
def phase(name,commands,wait=.5):
 p=subprocess.run(['docker','exec','--user','linuxgsm','-i','competitive_dev','python3','-c',(w/'phase-guest.py').read_text()],input=json.dumps({'phase':name,'commands':commands,'wait_seconds':wait}),capture_output=True,text=True,timeout=25);assert p.returncode==0,p.stderr
 (w/(name+'.json')).write_text(p.stdout);j=json.loads(p.stdout)
 assert not j['error_log_sizes'] and not j['crash_markers'] and not j['plugin_error_rows']
 assert not any('FAIL' in s or ('callback ' in s and 'pass=0' in s) for s in j['all_owner_trace']),j['trace']
 return j
def entered(seq):
 for _ in range(80):
  p=w/'fixture-events.json';j=json.loads(p.read_text()) if p.exists() else {}
  if '/hold/'+str(seq) in j:assert 'released' not in j['/hold/'+str(seq)];return
  time.sleep(.05)
 raise AssertionError('Fixture did not observe held request')
def release(seq):
 assert not(w/('release-'+str(seq))).exists();(w/('release-'+str(seq))).write_text('release\n')
def callback(j,seq):return any('callback sequence='+str(seq)+' context=1 pass=1' in s for s in j['all_owner_trace'])
def completion(name,seq,commands=[]):
 j=phase(name,commands,1.5)
 if not callback(j,seq):
  for retry in range(6):
   j=phase(name+'-retry-'+str(retry),['sm_sw_owner status'],1)
   if callback(j,seq):break
  else:raise AssertionError('Expected positive callback missing')
 assert j['owner_HTTP_handle_count']==0 and len(j['owner_plugin_rows'])==1
 return j
j=phase('baseline',['steamworks_debug 1','sm_sw_owner status']);assert j['owner_HTTP_handle_count']==0 and len(j['owner_plugin_rows'])==1
j=phase('positive-held',['sm_sw_owner hold 11']);entered(11);assert j['owner_HTTP_handle_count']==1 and not any('callback ' in s for s in j['trace']);release(11);completion('positive-completed',11)
print('POSITIVE_HELD_CALLBACK_AND_CLOSE_PASSED',flush=True)
for cycle,seq in [(1,21),(2,22)]:
 j=phase('held-'+str(cycle),['sm_sw_owner hold '+str(seq)]);entered(seq);assert j['owner_HTTP_handle_count']==1
 before=sum('callback ' in s for s in j['all_owner_trace'])
 j=phase('unloaded-'+str(cycle),['sm plugins unload steamworks_pending_owner.smx']);assert not j['owner_plugin_rows'] and j['owner_HTTP_handle_count']==0
 assert any('ended pending=1' in s and 'sequence='+str(seq) in s for s in j['trace'])
 assert sum('callback ' in s for s in j['all_owner_trace'])==before
 # Reuse the same plugin filename/VM entry points before releasing old response.
 j=phase('reloaded-'+str(cycle),['sm plugins load steamworks_pending_owner.smx','sm_sw_owner status']);assert len(j['owner_plugin_rows'])==1 and j['owner_HTTP_handle_count']==0
 completion('fresh-before-release-'+str(cycle),100+seq,['sm_sw_owner probe '+str(100+seq)])
 release(seq)
 j=phase('released-observation-'+str(cycle),['sm_sw_owner status'],7)
 assert j['owner_HTTP_handle_count']==0 and len(j['owner_plugin_rows'])==1
 assert sum('callback sequence='+str(seq)+' ' in s for s in j['all_owner_trace'])==0
 assert sum('callback ' in s for s in j['all_owner_trace'])==before+1
 completion('fresh-after-release-'+str(cycle),200+seq,['sm_sw_owner probe '+str(200+seq)])
 print('OWNER_UNLOAD_RELOAD_CYCLE_'+str(cycle)+'_PASSED',flush=True)
j=phase('final-unload',['sm plugins unload steamworks_pending_owner.smx']);assert not j['owner_plugin_rows'] and j['owner_HTTP_handle_count']==0
p=subprocess.run(['docker','exec','--user','linuxgsm','competitive_dev','python3','-c',(w/'collect-guest.py').read_text()],capture_output=True,text=True);assert p.returncode==0,p.stderr;(w/'final-modules.json').write_text(p.stdout)
a=json.loads((w/'initial-modules.json').read_text());b=json.loads(p.stdout);assert (a['lab_pid'],a['process_start_ticks'])==(b['lab_pid'],b['process_start_ticks'])
assert {m['path']:m['sha256'] for m in a['modules']}=={m['path']:m['sha256'] for m in b['modules']}
events=json.loads((w/'fixture-events.json').read_text());assert all(0<events['/hold/'+str(seq)]['released']-events['/hold/'+str(seq)]['entered']<60 for seq in [11,21,22])
(w/'checks.json').write_text(json.dumps({'passed':True,'PID':a['lab_pid'],'same_PID_start_ticks_and_mapped_modules':True,'positive_held_callback_and_close':True,'pending_owner_unload_cycles':2,'reload_before_old_response_release':True,'fresh_probe_callbacks':4,'unloaded_owner_callbacks':0,'visible_owner_HTTP_handle_counts_each_cycle':[0,1,0,0],'post_release_observation_seconds_each_cycle':7,'raw_handle_dumps_removed':True,'global_memory_or_SDK_allocation_leak_claim':False},indent=2)+'\n')
print('OWNER_UNLOAD_ACCEPTANCE_PASSED',flush=True)
