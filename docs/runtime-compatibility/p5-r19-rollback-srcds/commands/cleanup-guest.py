from pathlib import Path
import datetime,hashlib,json,shutil,time
root=Path('/data/serverfiles/left4dead2');lab=root/'addons/sourcemod_acceptance';baseline=json.loads((lab/'recovery/baseline.json').read_text())
console=Path('/data/log/console/l4d2server-console.log')
for _ in range(200):
 lines=console.read_text(errors='replace').splitlines()
 if any('Connection to Steam servers successful.' in s for s in lines):break
 time.sleep(.25)
else:raise SystemExit('Original server not Steam-connected')
processes=[]
for p in Path('/proc').glob('[0-9]*/comm'):
 try:
  if p.read_text().strip()!='srcds_linux':continue
  args=(p.parent/'cmdline').read_bytes().split(b'\0');maps=(p.parent/'maps').read_text()
  processes.append({'pid':int(p.parent.name),'original_selector':b'addons/sourcemod1' in args,'original_extension_mapped':str(root/'addons/sourcemod/extensions/steamworks.ext.so') in maps,'lab_extension_mapped':'sourcemod_acceptance' in maps})
 except OSError:pass
assert len(processes)==1 and all(p['original_selector'] and p['original_extension_mapped'] and not p['lab_extension_mapped'] for p in processes)
for p,h in baseline['original_files_sha256'].items():assert hashlib.sha256(Path(p).read_bytes()).hexdigest()==h,p
cfgfiles={str(p.relative_to(root/'cfg')):hashlib.sha256(p.read_bytes()).hexdigest() for p in (root/'cfg').rglob('*.cfg') if p.is_file()}
saved=json.loads((lab/'recovery/cfg-tree.json').read_text());assert cfgfiles==saved,'Global CFG tree differs; recovery retained'
shutil.rmtree(lab)
print(json.dumps({'timestamp_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'original_files_sha256':baseline['original_files_sha256'],'processes':processes,'original_server_steam_connected':True,'lab_removed':not lab.exists(),'global_cfg_files_verified':len(cfgfiles),'global_cfg_tree_sha256':hashlib.sha256(json.dumps(cfgfiles,sort_keys=True).encode()).hexdigest(),'global_cfg_tree_preserved':True,'game_cfg_modified':False,'production_touched':False},indent=2))
