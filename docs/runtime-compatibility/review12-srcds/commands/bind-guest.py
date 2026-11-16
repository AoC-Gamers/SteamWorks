from pathlib import Path
import json,sys,hashlib,subprocess
cfg=json.load(sys.stdin);lab=Path('/data/serverfiles/left4dead2/addons/sourcemod_acceptance');f=lab/'extensions/steamworks.ext.so';assert hashlib.sha256(f.read_bytes()).hexdigest()==cfg['candidate_sha256']
pids=[]
for p in Path('/proc').glob('[0-9]*'):
 try:
  if (p/'comm').read_text().strip()=='srcds_linux' and b'addons/sourcemod_acceptance' in (p/'cmdline').read_bytes().split(b'\0'):pids.append(p)
 except OSError:pass
assert len(pids)==1;pid=pids[0];maps=[];own=[]
for line in (pid/'maps').read_text().splitlines():
 a=line.split();start,end=[int(n,16) for n in a[0].split('-')];rec={'start':start,'end':end,'perms':a[1],'offset':int(a[2],16)};maps.append(rec)
 if len(a)>5 and a[5]==str(f):
  assert int(a[4])==f.stat().st_ino;own.append(rec)
assert own;base=min(a['start']-a['offset'] for a in own);slot=base+cfg['resolver_slot_rva'];getter=base+cfg['getter']['rva']
assert any(a['start']<=slot and slot+4<=a['end'] and 'w' in a['perms'] for a in own)
assert any(a['start']<=getter and getter+cfg['getter']['size']<=a['end'] and 'x' in a['perms'] for a in own)
if cfg.get('local'):
 hooks=base+cfg['hooks_slot_rva']; attach=base+cfg['attach']['rva']
 assert any(a['start']<=hooks and hooks+4<=a['end'] and 'w' in a['perms'] for a in own)
 assert any(a['start']<=attach and attach+cfg['attach']['size']<=a['end'] and 'x' in a['perms'] for a in own)
 command='sm_sw_buffers_gc_local '+' '.join(format(a,'x') for a in [slot,getter,hooks,attach])
else:
 command='sm_sw_buffers_gc '+' '.join(format(a,'x') for a in [slot,getter])
p=subprocess.run(['python3','-c',cfg['phase_code']],input=json.dumps({'phase':cfg['phase'],'commands':[command],'wait_seconds':1}),capture_output=True,text=True);assert p.returncode==0,p.stderr
j=json.loads(p.stdout);j['binding']={'candidate_sha256':cfg['candidate_sha256'],'executable_getter_and_writable_slot_verified':True,'mapped_file_inode_verified':True,'exact_binary_profile':True};print(json.dumps(j,indent=2))
