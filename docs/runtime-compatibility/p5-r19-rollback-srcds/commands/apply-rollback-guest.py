from pathlib import Path
import json,hashlib,zipfile,datetime,os
lab=Path('/data/serverfiles/left4dead2/addons/sourcemod_acceptance');cfg=Path('/data/config-lgsm/l4d2server/l4d2server.cfg');recovery=lab/'recovery'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
for p in Path('/proc').glob('[0-9]*/comm'):
 try:assert p.read_text().strip()!='srcds_linux','Refusing write while SRCDS is running'
 except OSError:pass
meta=json.loads((recovery/'baseline.json').read_text());assert sha(cfg)==meta['selected_sha256']
m=json.loads((recovery/'archive-manifest.json').read_text());archive=recovery/'rollback.zip';assert sha(archive)==m['archives']['steamworks-p5-r19-competitive-dev-rollback.zip']
owned={'extensions/steamworks.ext.so','scripting/include/steamworks.inc','gamedata/steamworks.txt'}
assert set(m['rollback_files'])|set(m['rollback_absent_paths'])==owned
cfgfiles=json.loads((recovery/'cfg-tree.json').read_text());root=lab.parent.parent;assert all(sha(root/'cfg'/n)==h for n,h in cfgfiles.items())
before={n:sha(lab/n) for n in ['extensions/steamworks.ext.so','scripting/include/steamworks.inc','configs/core.cfg','configs/restore-sentinel.cfg']}
assert before['extensions/steamworks.ext.so']==m['candidate_files']['addons/sourcemod/extensions/steamworks.ext.so']
# Deliberate file-contract witness, introduced ONLY after candidate shutdown.
game=lab/'gamedata/steamworks.txt';assert not game.exists();game.write_text('"Games" { "#default" { } }\n');witness=sha(game)
with zipfile.ZipFile(archive) as z:
 assert set(z.namelist())=={'addons/sourcemod/'+n for n in m['rollback_files']}
 saved={n:z.read('addons/sourcemod/'+n) for n in m['rollback_files']}
 assert all(hashlib.sha256(data).hexdigest()==m['rollback_files'][n] for n,data in saved.items())
 for n in owned:
  p=lab/n
  assert all(not parent.is_symlink() for parent in [p,*p.parents]),n
 for n,data in saved.items():
  p=lab/n;temp=p.with_name(p.name+'.restore-tmp');assert not temp.exists();temp.write_bytes(data);os.replace(temp,p)
 for n in m['rollback_absent_paths']:(lab/n).unlink(missing_ok=True)
after={n:sha(lab/n) for n in m['rollback_files']}
assert after==m['rollback_files']
assert all(not (lab/n).exists() for n in m['rollback_absent_paths'])
assert sha(lab/'configs/core.cfg')==before['configs/core.cfg'] and sha(lab/'configs/restore-sentinel.cfg')==before['configs/restore-sentinel.cfg']
assert sha(cfg)==meta['selected_sha256']
print(json.dumps({'timestamp_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'archive_sha256':sha(archive),'before_sha256':before,'restored_sha256':after,'absent_paths_verified':m['rollback_absent_paths'],'gamedata_witness_sha256':witness,'gamedata_witness_created_after_candidate_stop':True,'gamedata_witness_not_generated_by_candidate':True,'same_sm_basepath':'addons/sourcemod_acceptance','selector_unchanged':True,'unrelated_core_config_and_sentinel_preserved':True,'global_cfg_tree_preserved':all(sha(root/'cfg'/n)==h for n,h in cfgfiles.items()),'restored_directly_from_guest_ZIP':True,'original_global_files_unchanged':all(sha(Path(p))==h for p,h in meta['original_files_sha256'].items() if str(cfg)!=p)},indent=2))
