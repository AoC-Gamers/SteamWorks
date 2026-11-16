from pathlib import Path
import base64,hashlib,json,re,shutil,sys
root=Path('/data/serverfiles/left4dead2');lab=root/'addons/sourcemod_acceptance'
assert not lab.exists(),'Lab already exists'
for p in Path('/proc').glob('[0-9]*/comm'):
 try: assert p.read_text().strip()!='srcds_linux','Stop DEV before snapshot'
 except OSError: pass
payload=json.load(sys.stdin)
lab.mkdir(); recovery=lab/'recovery';recovery.mkdir(mode=0o700)
cfg=Path('/data/config-lgsm/l4d2server/l4d2server.cfg');original=cfg.read_bytes()
pattern=re.compile(r'(?m)^(\s*basepath\s*=\s*)([^\r\n]+)$')
text=original.decode();matches=list(pattern.finditer(text));assert len(matches)==1
assert matches[0][2].strip().strip('"\'')=='addons/sourcemod1'
selected=pattern.sub(lambda m:m[1]+'"addons/sourcemod_acceptance"',text).encode()
for name,data in [('original.cfg',original),('selected.cfg',selected)]:
 p=recovery/name;p.write_bytes(data);p.chmod(0o600)
files=[cfg,root/'addons/sourcemod/extensions/steamworks.ext.so',root/'cfg/sourcemod/l4d2_familyshare_core.cfg',root/'cfg/sourcemod/AoC.cfg']
meta={'original_files_sha256':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in files},'original_selector':'addons/sourcemod1','selected_selector':'addons/sourcemod_acceptance','selected_sha256':hashlib.sha256(selected).hexdigest()}
(recovery/'baseline.json').write_text(json.dumps(meta,indent=2))
for name in ['bin','gamedata','translations','configs','extensions','plugins']:
 shutil.copytree(root/'addons/sourcemod'/name,lab/name)
shutil.copytree(root/'addons/sourcemod1/data',lab/'data')
(lab/'logs').mkdir();(lab/'scripting/include').mkdir(parents=True)
core=lab/'configs/core.cfg';s=core.read_text();s,n=re.subn(r'("DisableAutoUpdate"\s*)"[^"]*"',lambda m:m[1]+'"yes"',s);assert n==1;core.write_text(s)
for name,data in payload.items():
 target=lab/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(base64.b64decode(data))
import zipfile,os
m=json.loads((recovery/'archive-manifest.json').read_text());archive=recovery/'candidate.zip'
assert hashlib.sha256(archive.read_bytes()).hexdigest()==m['archives']['steamworks-p5-r9-linux-x86-candidate.zip']
# Original files must match the rollback before installing any candidate bytes.
assert all(hashlib.sha256((root/'addons/sourcemod'/n).read_bytes()).hexdigest()==h for n,h in m['rollback_files'].items())
assert all(not (root/'addons/sourcemod'/n).exists() for n in m['rollback_absent_paths'])
with zipfile.ZipFile(archive) as z:
 assert z.testzip() is None and set(z.namelist())==set(m['candidate_files'])
 assert all(hashlib.sha256(z.read(n)).hexdigest()==h for n,h in m['candidate_files'].items())
 runtime={n:h for n,h in m['candidate_files'].items() if n.startswith('addons/')}
 assert set(runtime)=={'addons/sourcemod/extensions/steamworks.ext.so','addons/sourcemod/scripting/include/steamworks.inc'}
 for n,h in runtime.items():
  p=lab/n.removeprefix('addons/sourcemod/')
  assert all(not parent.is_symlink() for parent in [p,*p.parents])
  temp=p.with_name(p.name+'.install-tmp');assert not temp.exists();temp.write_bytes(z.read(n));os.replace(temp,p)
  assert hashlib.sha256(p.read_bytes()).hexdigest()==h
installation={'archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'installed_directly_from_guest_ZIP':True,'all_archive_members_verified':len(m['candidate_files']),'runtime_paths':runtime,'no_SRCDS_running_during_install':True,'original_baseline_matches_rollback_ZIP':True,'candidate_gamedata_absent':not (lab/'gamedata/steamworks.txt').exists(),'only_runtime_payload_installed':True}
(recovery/'installation.json').write_text(json.dumps(installation,indent=2))
meta['candidate_installation']=installation
cfg.write_bytes(selected)
print(json.dumps(meta,indent=2))
