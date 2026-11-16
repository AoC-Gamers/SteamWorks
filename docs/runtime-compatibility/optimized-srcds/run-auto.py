from pathlib import Path
import json,subprocess,time
r=Path('.build/optimized-srcds')
def phase(name,commands,wait=9):
 p=subprocess.run(['python3',str(r/'phase.py'),name,json.dumps(commands),str(wait)],capture_output=True,text=True,timeout=55)
 (r/(name+'-runner.log')).write_text(p.stdout+p.stderr);assert p.returncode==0,p.stderr
 j=json.loads((r/(name+'.json')).read_text());print(name,'passed',flush=True);return j
j=phase('initial-http',['sm_sw_accept http'])
assert sum('complete case=' in s and 'pass=1' in s for s in j['trace'])==8
j=phase('reinit-1',['map c2m2_fairgrounds'],12)
assert j['engine_connections']==1 and sum('connected_event' in s for s in j['trace'])==1
j=phase('after-reinit-1-http',['sm_sw_accept http'])
assert sum('complete case=' in s and 'pass=1' in s for s in j['trace'])==8
j=phase('pending-queued',['sm_sw_lifecycle_pending queue'],.5)
assert any('queued configured=1 sent=1' in s for s in j['trace'])
assert (r/'hold-entered.json').exists()
j=phase('reinit-2-pending',['map c2m1_highway','sm_sw_lifecycle_pending check'],12)
assert j['engine_connections']==1 and sum('connected_event' in s for s in j['trace'])==1
assert any('retained pass=1 owned=1 callbacks=0' in s for s in j['trace'])
j=phase('after-reinit-2-http',['sm_sw_accept http'])
assert sum('complete case=' in s and 'pass=1' in s for s in j['trace'])==8
assert not (r/'hold-released.json').exists()
(r/'release-hold').write_text('release\n')
j=phase('after-held-response',['sm_sw_lifecycle_pending check'],7)
assert any('retained pass=1 owned=1 callbacks=0' in s for s in j['trace'])
assert not any('stale_callback' in s for s in j['trace'])
j=phase('pending-close',['sm_sw_lifecycle_pending close','sm_sw_accept cancel','sm_sw_accept stats','sm_sw_accept probe'],7)
assert any('closed callbacks=0' in s for s in j['trace'])
assert any('stats_failure pass=1' in s for s in j['trace'])
assert any('complete case=8 pass=1' in s for s in j['trace'])
assert any('completed=25 failed=0 headers=3 streamBytes=18' in s for s in j['trace'])
(r/'automatic-checks.json').write_text(json.dumps({'passed':True,'full_reinitializations':2,'same_process':True,'HTTP_completions':25,'stream_bytes':18,'stream_header_events':3,'held_request_crossed_shutdown':True,'stale_callbacks':0,'pending_handle_closed':True,'cancelled_callbacks':0,'stats_failure_outputs_preserved':True},indent=2)+'\n')
print('AUTOMATIC_ACCEPTANCE_PASSED',flush=True)
