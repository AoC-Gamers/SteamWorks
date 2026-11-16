from pathlib import Path
import base64,hashlib,json,re,shutil,sys
root=Path('/data/serverfiles/left4dead2'); lab=root/'addons/sourcemod_acceptance'
assert not lab.exists()
for p in Path('/proc').glob('[0-9]*/comm'):
 try: assert p.read_text().strip()!='srcds_linux','DEV must be stopped before snapshot'
 except OSError: pass
payload=json.load(sys.stdin); lab.mkdir(); recovery=lab/'recovery'; recovery.mkdir(mode=0o700)
cfg=Path('/data/config-lgsm/l4d2server/l4d2server.cfg'); original=cfg.read_bytes(); pattern=re.compile(r'(?m)^(\s*basepath\s*=\s*)([^\r\n]+)$')
matches=list(pattern.finditer(original.decode())); assert len(matches)==1 and matches[0][2].strip().strip('"\'')=='addons/sourcemod1'
selected=pattern.sub(lambda m:m[1]+'"addons/sourcemod_acceptance"',original.decode()).encode()
for n,data in [('original.cfg',original),('selected.cfg',selected)]:
 p=recovery/n;p.write_bytes(data);p.chmod(0o600)
protected=[cfg,root/'addons/sourcemod/extensions/steamworks.ext.so',root/'cfg/sourcemod/l4d2_familyshare_core.cfg',root/'cfg/sourcemod/AoC.cfg']
meta={'original_files_sha256':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in protected},'selected_sha256':hashlib.sha256(selected).hexdigest(),'original_selector':'addons/sourcemod1','selected_selector':'addons/sourcemod_acceptance'}
(recovery/'baseline.json').write_text(json.dumps(meta,indent=2))
for n in ['bin','gamedata','translations','configs','extensions']:
 shutil.copytree(root/'addons/sourcemod'/n,lab/n)
shutil.copytree(root/'addons/sourcemod1/data',lab/'data')
for n in ['logs','plugins','scripting/include']:(lab/n).mkdir(parents=True,exist_ok=True)
core=lab/'configs/core.cfg'; s,n=re.subn(r'("DisableAutoUpdate"\s*)"[^"]*"',lambda m:m[1]+'"yes"',core.read_text());assert n==1;core.write_text(s)
for name,data in payload.items():
 p=lab/name; assert '..' not in Path(name).parts and not Path(name).is_absolute();p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(base64.b64decode(data))
meta['installed_sha256']={n:hashlib.sha256((lab/n).read_bytes()).hexdigest() for n in payload}
assert (lab/'extensions/steamworks.ext.so').read_bytes()[:5]==b'\x7fELF\x01'
assert all(p.suffix!='smx' for p in (lab/'plugins').iterdir() if p.name!='steamworks_buffer_acceptance.smx')
cfg.write_bytes(selected)
print(json.dumps(meta,indent=2))
