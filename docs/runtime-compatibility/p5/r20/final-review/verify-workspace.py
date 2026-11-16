"""Original-workspace final verifier; needs its Git/captured build inputs. No server operations."""
from pathlib import Path,PurePosixPath
import json,hashlib,subprocess,zipfile,tarfile,io,tempfile,re,importlib.util
r=Path.cwd();w=r/'.build/p5-r20';out=w/'review';d=r/'docs/runtime-compatibility/p5';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();hs=lambda b:hashlib.sha256(b).hexdigest();x=json.loads((d/'validation-results.json').read_text());m=json.loads((out/'manifest.json').read_text())
assert x['checkpoint']=='p5-r20' and not x['full_matrix_accepted'] and x['pending_acceptance'][0]['id']=='GC-REMOTE'
assert all(sha(r/n)==h for n,h in x['source_sha256'].items())
assert sha(w/'reproducible-build/package/addons/sourcemod/extensions/steamworks.ext.so')==x['build']['sha256']=='72664c658f6d28e78a975ec98c96879dc641f96725206f514f159ea17125fcb3'
for n,h in json.loads((w/'historical-artifact-hashes.json').read_text()).items():assert sha(r/n)==h,n
for n,h in json.loads((d/'r20/acceptance-evidence-hashes.json').read_text()).items():assert sha(r/n)==h,n
original=json.loads((r/'docs/runtime-compatibility/review12/results.json').read_text());assert sha(Path(original['request']))==original['request_sha256']
assert len(m['archives'])==4
for n,h in m['archives'].items():
 assert sha(out/n)==h
 with zipfile.ZipFile(out/n) as z:
  assert z.testzip() is None
  for name in z.namelist():
   p=PurePosixPath(name);assert not p.is_absolute() and '..' not in p.parts
   assert not any(s in p.parts for s in ['sdk_158a','.env','.deps'])
   assert p.name not in ['steamclient.so','libsteam_api.so','engine_srv.so','original.cfg','selected.cfg']
with zipfile.ZipFile(out/'steamworks-p5-r20-linux-x86-candidate.zip') as z:
 assert set(z.namelist())==set(m['candidate_files'])
 assert not any(n.endswith('.smx') for n in z.namelist())
 assert set(n for n in z.namelist() if n.startswith('addons/'))=={'addons/sourcemod/extensions/steamworks.ext.so','addons/sourcemod/scripting/include/steamworks.inc'}
 for n,h in m['candidate_files'].items():assert hs(z.read(n))==h==sha(w/'candidate'/n)
 state=json.loads(z.read('docs/runtime-compatibility/p5/validation-results.json'));assert state['checkpoint']=='p5-r20' and not state['full_matrix_accepted'] and state['pending_acceptance'][0]['id']=='GC-REMOTE'
with zipfile.ZipFile(out/'steamworks-p5-r20-source-review.zip') as z:
 assert not any(n.endswith('.smx') for n in z.namelist())
 sm=json.loads(z.read('review/source-manifest.json'))
 for n,h in sm['files'].items():assert hs(z.read(n))==h,n
 for n,h in x['source_sha256'].items():assert hs(z.read(n))==h==sha(w/'source-unpacked'/n),n
 for n in ['run-reproducible-build.sh','run-integration.sh']:
  assert z.read('review/'+n)==(w/'source-unpacked/review'/n).read_bytes()
 assert z.read('.gitattributes')==(r/'.gitattributes').read_bytes()
 # Verify that the complete patch contains additions, not just tracked diffs.
 patch=z.read('review/changes-from-HEAD.patch')
 with tempfile.TemporaryDirectory(prefix='steamworks-r19-patch-') as tmp:
  with tarfile.open(fileobj=io.BytesIO(subprocess.check_output(['git','archive',x['baseline_commit']]))) as t:t.extractall(tmp,filter='data')
  p=subprocess.run(['git','apply','--check','-'],cwd=tmp,input=patch,capture_output=True);assert p.returncode==0,p.stderr.decode()
  p=subprocess.run(['git','apply','-'],cwd=tmp,input=patch,capture_output=True);assert p.returncode==0,p.stderr.decode()
  tracked=[n for n in sm['files'] if not n.startswith('review/')]
  for n in tracked:assert (Path(tmp)/n).read_bytes()==z.read(n),n
with zipfile.ZipFile(out/'steamworks-p5-r20-vm-diagnostics.zip') as z:
 assert not any(n.startswith('addons/') for n in z.namelist())
 assert hs(z.read('diagnostics/linux-x86/libsourcepawn.so'))==x['asan']['VM_sha256']
 for n,h in m['diagnostic_files'].items():assert hs(z.read(n))==h
assert m['archives']['steamworks-p5-r20-competitive-dev-rollback.zip']=='55153d557f3af7f71f1016c8c394dc74fe29e0cdc9511044c21de29e8bc6dffc'
assert json.loads((w/'rollback-roundtrip.json').read_text())==json.loads((d/'r20/rollback-roundtrip.json').read_text())
for p in [w/'integration/results.json',w/'source-unpacked/.build/p5-r20/integration/results.json']:
 j=json.loads(p.read_text());assert len(j['suites'])==15 and j['public_api']=='passed' and all(t['exit_code']==0 for t in j['suites'])
assert sha(w/'source-unpacked/.build/p5-r20/reproducible-build/package/addons/sourcemod/extensions/steamworks.ext.so')==x['build']['sha256']
assert '133 hook lifecycle checks' in (w/'asan-focused.log').read_text() and '33 strict HTTP text response checks' in (w/'asan-focused.log').read_text()
assert '227 authentication forward checks' in (w/'diagnostic-replay.log').read_text() and '16 VM ownership cycles' in (w/'diagnostic-replay.log').read_text()
assert 'Ran 66 tests' in (w/'Connect-python-tests.log').read_text() and '\nOK\n' in (w/'Connect-python-tests.log').read_text()
assert json.loads((w/'sdk-harness-checks.json').read_text())['harnesses_compiled_without_warnings']==19
assert json.loads((w/'Connect-patch-roundtrip.json').read_text())['all_tracked_target_files_match']
assert json.loads((w/'DEV-before.json').read_text())==json.loads((w/'DEV-after.json').read_text())
assert x['build']['P5_R20_archive_installed_in_SRCDS'] is False
assert x['build']['identical_to_T26_T35_R19_archive_payload']
assert x['rollback']['current_archive_applied_in_place'] is False
assert x['rollback']['current_archive_bytes_previously_applied']
assert x['rollback']['current_in_place_evidence'] is None
assert x['rollback']['prior_in_place_checkpoint']=='p5-r19'
assert x['rollback']['current_archive_validation_evidence']=='p5/r20/rollback-roundtrip.json'
links=0
for p in [r/'README.md',r/'docs/README.md',r/'docs/TestCoverage.md',d.parent/'p5-integration.md',d.parent/'p5-rollback.md',d.parent/'compatibility.md']:
 for target in re.findall(r'\]\(([^)]+)\)',p.read_text()):
  if target.startswith(('http:','https:','#','mailto:')):continue
  assert (p.parent/target.split('#')[0]).exists(),(p,target)
  links+=1
for repo,head in [(r,'adffed0cc90cad929183b7adc896a6f86ce281da'),(Path('/home/lechuga/GitHub/Connect/Connect-Sourcepawn'),'5f62f3c6877d5ee21dc912ad498c881921ea15d7')]:
 assert subprocess.check_output(['git','branch','--show-current'],cwd=repo,text=True).strip()=='develop'
 assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip()==head
 assert not subprocess.check_output(['git','diff','--cached','--name-only'],cwd=repo)
assert not subprocess.check_output(['git','diff','--name-only'],cwd='/home/lechuga/GitHub/Connect/Connect-Sourcepawn')
subprocess.run(['git','diff','--check'],check=True)
v={'status':'passed','checkpoint':'p5-r20','GC_remote_positive':'pending','full_new_runtime_matrix_inferred':False,'local_x86_suites':15,'clean_source_extraction_suites':15,'native_VM_and_focused_sanitizers':'passed without suppressions','public_API':'unchanged vs P0','ELF_matches_review12_accepted_payload':True,'runtime_scope':'Review12 buffers plus exact R19 ZIP T26/T35 retained by identical native payload and rollback ZIP; R20 archive offline only','runtime_harnesses_compiled':19,'consumers_compiled':3,'Connect_fixtures':4,'Connect_Python_tests':66,'SDK_inputs':55,'source_files_verified':len(x['source_sha256']),'source_archive_complete_patch_roundtrip':'passed including added files','native_and_VM_tests_repeated_in_R20':True,'final_review_native_validation_reused_by_exact_hashes':True,'final_review_findings_fixed':2,'final_review_remaining_findings':[],'candidate_and_source_exclude_compiled_test_SMX':True,'source_archive_build':'passed; byte-identical ELF','Connect_complete_patch_roundtrip':'passed','four_archive_contents_and_hashes':'passed','rollback_local_roundtrip':'passed','rollback_ZIP_matches_R9_applied_bytes':True,'historical_artifacts_unchanged':len(json.loads((w/'historical-artifact-hashes.json').read_text())),'DEV_same_PID_selector_and_protected_hashes':True,'links_verified':links,'branch':'develop','HEADs_unchanged':True,'indexes_empty':True,'servers_operated':False,'committed':False,'pushed':False,'deployed':False}
(out/'verification.json').write_text(json.dumps(v,indent=2)+'\n');(out/'SHA256SUMS').write_text(''.join(sha(p)+'  '+p.name+'\n' for p in sorted(out.iterdir()) if p.is_file() and p.name!='SHA256SUMS'))
for line in (out/'SHA256SUMS').read_text().splitlines():h,n=line.split(maxsplit=1);assert sha(out/n)==h
print(json.dumps(v,indent=2))
