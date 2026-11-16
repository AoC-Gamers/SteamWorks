from pathlib import Path
import json,hashlib,re,subprocess
r=Path.cwd();w=r/'.build/review12-srcds';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
prior=json.loads((r/'docs/runtime-compatibility/review12/results.json').read_text())
for key in ['production_source_sha256','regression_source_sha256']:
 for n,h in prior[key].items():assert sha(r/n)==h,n
assert subprocess.check_output(['git','branch','--show-current'],text=True).strip()=='develop'
assert subprocess.check_output(['git','diff','--cached','--name-only'],text=True)==''
assert (r/'docs/runtime-compatibility/p5/validation-results.json').read_bytes()==subprocess.check_output(['git','show','HEAD:docs/runtime-compatibility/p5/validation-results.json'])
for n,h in json.loads((r/'.build/review12/historical-artifact-hashes.json').read_text()).items():assert sha(r/n)==h,n
assert sha(Path(prior['request']))==prior['request_sha256']
phases=['HTTP-final','GC-final','GC-local-final','GC-restored-real','newgame','HTTP-after-map','GC-after-map','GC-local-after-map','GC-restored-after-map'];records={n:json.loads((w/(n+'.json')).read_text()) for n in phases}
pids=set();checks=0
for name,j in records.items():
 assert not j['error_log_metadata'] and not j['crash_markers'],name
 assert not any('pass=0' in s or 'GC pending_' in s for s in j['console_trace']),name
 assert not any('skipped_' in s for s in j['driver_trace']),name
 pids.add(tuple((p['pid'],p['start_ticks']) for p in j['processes']))
 checks+=sum(' check=' in s for s in j['console_trace'])
 if 'HTTP' in name:
  assert sum('HTTP_full_read_then_bounded_prefix_NUL_and_canaries pass=1' in s for s in j['console_trace'])==18
  assert sum('HTTP_pending_outputs_unchanged pass=1' in s for s in j['console_trace'])==3
 elif name.startswith('GC-local'):
  assert len(j['driver_trace'])==5 and all('original_calls=1 provider_bytes_and_outputs_preserved=1 real_target_restored=1' in s for s in j['driver_trace'])
 elif name.startswith('GC'):
  assert len(j['driver_trace'])==20 and all('buffer_unchanged=1' in s for s in j['driver_trace'])
assert len(pids)==1 and checks==372,(pids,checks)
a=json.loads((w/'initial-modules.json').read_text()) # First-stage candidate also verified.
b=json.loads((w/'final-modules.json').read_text())
assert all(any(m['file_name']=='steamworks.ext.so' and m['sha256']==prior['optimized_build']['sha256'] for m in j['modules']) for j in [a,b])
assert any(m['file_name']=='steamworks_buffer_fixture.ext.so' and m['sha256']==sha(w/'fixture/steamworks_buffer_fixture.ext.so') for m in b['modules'])
assert 'warning' not in (w/'harness-compile.log').read_text().lower()
assert (w/'fixture-build-final.log').read_text()==''
result={'status':'passed_scoped_SRCDS_x86_acceptance','checks':checks,'failures':0,'http_body_capacity_combinations':36,'HTTP_completions':6,'HTTP_pending_failure_output_preservation':6,'GC_real_empty_queue_calls':80,'GC_local_provider_calls':10,'same_PID_across_newgame':True,'process_identity':list(pids)[0],'production_sources_and_public_include_unchanged_since_local_review':True,'historical_artifacts_verified_unchanged':155,'original_OneDrive_request_unchanged':True,'historical_P5_matrix_unchanged':True,'real_remote_GC_message_received':False,'remote_CI':'not_run_for_uncommitted_changes','production_deployed':False}
(w/'checks.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
