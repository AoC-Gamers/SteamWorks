"""Audit R20 checkpoint references with disposable copies, never edit evidence."""
from pathlib import Path
import hashlib,json,tempfile,shutil
r=Path.cwd();w=r/'.build/p5-r20';fr=r/'.build/p5-r20-final-review';d=r/'docs/runtime-compatibility/p5';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
current=json.loads((d/'r20/acceptance-evidence-hashes.json').read_text());legacy=json.loads((d/'r19/acceptance-evidence-hashes.json').read_text());assert len(current)==149
key='docs/runtime-compatibility/p5-r19-rollback-srcds/results.json';assert key in current and 'p5-r19-rollback-srcds/results.json' not in legacy
source=(w/'verify-final.py').read_text();assert "(d/'r20/acceptance-evidence-hashes.json')" in source and 'assert sha(r/n)==h,n' in source and "(d/'r20/rollback-roundtrip.json')" in source
for n,h in current.items():assert sha(r/n)==h,n
# Execute the exact guard statements extracted from the corrected verifier,
# against temporary trees where just one evidence item is changed.
evidence_line=next(s for s in source.splitlines() if "(d/'r20/acceptance-evidence-hashes.json')" in s)
rollback_line=next(s for s in source.splitlines() if "(d/'r20/rollback-roundtrip.json')" in s)
results=[]
with tempfile.TemporaryDirectory(prefix='steamworks-r20-guard-') as td:
 temp=Path(td);doc=temp/'docs/runtime-compatibility/p5';(doc/'r20').mkdir(parents=True)
 for n,h in current.items():
  p=temp/n;p.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(r/n,p)
 (doc/'r20/acceptance-evidence-hashes.json').write_text(json.dumps(current))
 scope={'json':json,'sha':sha,'r':temp,'d':doc}
 exec(evidence_line,scope)
 (temp/key).write_bytes((temp/key).read_bytes()+b'\n')
 try:exec(evidence_line,scope)
 except AssertionError:results.append('altered_T26_T35_evidence_rejected')
 else:raise AssertionError('Evidence guard accepted a changed proof')
 real=json.loads((w/'rollback-roundtrip.json').read_text());(temp/'work').mkdir();(temp/'work/rollback-roundtrip.json').write_text(json.dumps(real));(doc/'r20/rollback-roundtrip.json').write_text(json.dumps(real));scope={'json':json,'w':temp/'work','d':doc};exec(rollback_line,scope)
 changed=dict(real);changed['foreign_config_and_plugin_preserved']=False;(doc/'r20/rollback-roundtrip.json').write_text(json.dumps(changed))
 try:exec(rollback_line,scope)
 except AssertionError:results.append('altered_current_R20_roundtrip_rejected')
 else:raise AssertionError('Rollback guard accepted changed current results')
assert len(results)==2
result={'status':'passed','current_evidence_map_entries':len(current),'negative_controls':results,'historical_evidence_guard_kept':True,'real_files_modified':False,'old_reference_gap':'legacy R19 evidence map excludes later T26/T35 proof; old roundtrip reference bypasses the current R20 documentary result','product_tests_changed':False}
(fr/'checkpoint-guard-checks.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
