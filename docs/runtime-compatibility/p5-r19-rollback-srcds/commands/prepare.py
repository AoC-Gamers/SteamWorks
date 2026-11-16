from pathlib import Path
import json,hashlib,zipfile,base64,subprocess,re,datetime
r=Path.cwd();w=r/'.build/p5-r19-rollback-srcds';review=r/'.build/p5-r19/review';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert subprocess.check_output(['git','branch','--show-current'],text=True).strip()=='develop'
assert subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()=='adffed0cc90cad929183b7adc896a6f86ce281da'
assert not subprocess.check_output(['git','status','--porcelain'])
x=json.loads((r/'docs/runtime-compatibility/p5/validation-results.json').read_text());assert x['pending_acceptance'][0]['id']=='GC-REMOTE' and not x['full_matrix_accepted'];assert all(sha(r/n)==h for n,h in x['source_sha256'].items())
frozen={}
for sums in (r/'.build').glob('*/review/SHA256SUMS'):
 for line in sums.read_text().splitlines():
  h,n=line.split(maxsplit=1);p=sums.parent/n;assert sha(p)==h,p;frozen[str(p.relative_to(r))]=h
(w/'historical-artifact-hashes.json').write_text(json.dumps(frozen,indent=2)+'\n')
m=json.loads((review/'manifest.json').read_text());assert m['current_candidate_ELF_sha256']=='72664c658f6d28e78a975ec98c96879dc641f96725206f514f159ea17125fcb3'
payload={};files={}
for role,name in [('candidate','steamworks-p5-r19-linux-x86-candidate.zip'),('rollback','steamworks-p5-r19-competitive-dev-rollback.zip')]:
 archive=review/name;assert sha(archive)==m['archives'][name]
 with zipfile.ZipFile(archive) as z:
  expected=m['candidate_files'] if role=='candidate' else {'addons/sourcemod/'+n:h for n,h in m['rollback_files'].items()}
  assert z.testzip() is None and set(z.namelist())==set(expected) and all(hashlib.sha256(z.read(n)).hexdigest()==h for n,h in expected.items())
  for n in ['extensions/steamworks.ext.so','scripting/include/steamworks.inc']:
   p=w/'payload'/role/n;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(z.read('addons/sourcemod/'+n))
 payload['recovery/'+role+'.zip']=base64.b64encode(archive.read_bytes()).decode();files[role+'_archive_sha256']=sha(archive)
payload['recovery/archive-manifest.json']=base64.b64encode((review/'manifest.json').read_bytes()).decode();payload['configs/restore-sentinel.cfg']=base64.b64encode(b'// isolated foreign restore sentinel\n').decode()
compiler=r.parent/'L4D2-Family-Share/deps/sourcemod-linux/addons/sourcemod/scripting/spcomp64';sm_inc=compiler.parent/'include'
for role in ['candidate','rollback']:
 out=w/('harness-'+role+'.smx');p=subprocess.run([str(compiler),str(r/'tests/runtime/steamworks_rollback_acceptance.sp'),'-i'+str(w/'payload'/role/'scripting/include'),'-i'+str(sm_inc),'-o'+str(out)],capture_output=True,text=True);(w/('compile-'+role+'.log')).write_text(p.stdout+p.stderr);assert p.returncode==0 and not re.search(r'warning \d',p.stdout+p.stderr),p.stdout+p.stderr
assert sha(w/'harness-candidate.smx')==sha(w/'harness-rollback.smx');payload['plugins/steamworks_rollback_acceptance.smx']=base64.b64encode((w/'harness-candidate.smx').read_bytes()).decode()
(w/'payload.json').write_text(json.dumps(payload));files.update(candidate_files=m['candidate_files'],rollback_files=m['rollback_files'],rollback_absent_paths=m['rollback_absent_paths'],HEAD=x['baseline_commit'],accepted_source_commit='adffed0cc90cad929183b7adc896a6f86ce281da');(w/'archives.json').write_text(json.dumps(files,indent=2)+'\n')
for n in ['setup-guest.py','apply-rollback-guest.py','collect-guest.py']:(w/n).write_text((r/'.build/p5-r9-rollback-srcds'/n).read_text().replace('p5-r9','p5-r19'))
s=(w/'setup-guest.py').read_text().replace("for name in ['bin','gamedata','translations','configs','extensions','plugins']:","for name in ['bin','gamedata','translations','configs','extensions']:")
s=s.replace("(lab/'logs').mkdir();(lab/'scripting/include').mkdir(parents=True)","(lab/'logs').mkdir();(lab/'plugins').mkdir();(lab/'scripting/include').mkdir(parents=True)")
s=s.replace("(recovery/'baseline.json').write_text(json.dumps(meta,indent=2))",'''cfgfiles={str(p.relative_to(root/'cfg')):hashlib.sha256(p.read_bytes()).hexdigest() for p in (root/'cfg').rglob('*.cfg') if p.is_file()}
(recovery/'cfg-tree.json').write_text(json.dumps(cfgfiles,sort_keys=True));(recovery/'cfg-tree.json').chmod(0o600)
meta['global_cfg_tree']={'count':len(cfgfiles),'sha256':hashlib.sha256(json.dumps(cfgfiles,sort_keys=True).encode()).hexdigest()}
(recovery/'baseline.json').write_text(json.dumps(meta,indent=2))''')
(w/'setup-guest.py').write_text(s)
s=(w/'apply-rollback-guest.py').read_text().replace("before={n:sha(lab/n)","cfgfiles=json.loads((recovery/'cfg-tree.json').read_text());root=lab.parent.parent;assert all(sha(root/'cfg'/n)==h for n,h in cfgfiles.items())\nbefore={n:sha(lab/n)")
s=s.replace("'restored_directly_from_guest_ZIP':True","'global_cfg_tree_preserved':all(sha(root/'cfg'/n)==h for n,h in cfgfiles.items()),'restored_directly_from_guest_ZIP':True")
(w/'apply-rollback-guest.py').write_text(s)
(w/'fixture-run.py').write_text((r/'.build/review12-srcds/fixture-run.py').read_text().replace('review12-srcds','p5-r19-rollback-srcds'))
(w/'stop-guest.py').write_text((r/'.build/p5-r5-rollback-srcds/stop-guest.py').read_text().replace('range(100)','range(250)'))
(w/'smoke-guest.py').write_text((r/'.build/p5-r5-rollback-srcds/smoke-guest.py').read_text().replace('range(100)','range(160)'))
print(json.dumps({'candidate_ZIP':files['candidate_archive_sha256'],'rollback_ZIP':files['rollback_archive_sha256'],'baseline_candidate_harness_identical':True,'prior_artifact_files_frozen':len(frozen),'scope':'minimal isolated observer; no production consumers loaded'},indent=2))
