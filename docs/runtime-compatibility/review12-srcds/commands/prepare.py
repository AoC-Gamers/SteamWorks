from pathlib import Path
import base64,subprocess,hashlib,json
r=Path.cwd();w=r/'.build/review12-srcds';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert subprocess.check_output(['git','branch','--show-current'],text=True).strip()=='develop'
assert subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()=='a0016a23adc70dd9c1bdf2bf22bf8ae0e685dbc6'
f=r/'.build/review12/build/package/addons/sourcemod/extensions/steamworks.ext.so'; assert sha(f)=='72664c658f6d28e78a975ec98c96879dc641f96725206f514f159ea17125fcb3'
prior=json.loads((r/'docs/runtime-compatibility/review12/results.json').read_text())
# Verify every file in the existing correction evidence, before operating DEV.
for key in ['source_sha256','test_sha256']:
 for n,h in prior.get(key,{}).items():assert sha(r/n)==h,n
payload={n:base64.b64encode(p.read_bytes()).decode() for n,p in {'extensions/steamworks.ext.so':f,'scripting/include/steamworks.inc':r/'scripting/include/steamworks.inc','extensions/steamworks_buffer_fixture.ext.so':w/'fixture/steamworks_buffer_fixture.ext.so','plugins/steamworks_buffer_acceptance.smx':w/'steamworks_buffer_acceptance.smx'}.items()}
payload['extensions/steamworks_buffer_fixture.autoload']=''
(w/'payload.json').write_text(json.dumps(payload))
asm=subprocess.check_output(['objdump','-d','--disassemble=_ZN12_GLOBAL__N_1L14sm_IsConnectedEPN10SourcePawn14IPluginContextEPKi',str(f)],text=True);(w/'owner-slot-disassembly.txt').write_text(asm)
assert '6ea6:' in asm and 'add    $0x2c14e,%ebx' in asm and 'lea    0x122c(%ebx),%eax' in asm and 'push   0x18(%eax)' in asm
symbols={}
for line in subprocess.check_output(['nm','-S',str(f)],text=True).splitlines():
 a=line.split()
 if len(a)==4:symbols[a[3]]={'rva':int(a[0],16),'size':int(a[1],16)}
assert symbols['g_SteamWorks']['rva']==0x6ea6+0x2c14e+0x122c==0x34220
(w/'getter-profile.json').write_text(json.dumps({'candidate_sha256':sha(f),'resolver_slot_rva':0x34238,'getter':symbols['_ZN20SteamWorksGameServer18GetGameCoordinatorEv'],'owner_slot_provenance':'Exact hashed ELF sm_IsConnected PIC and typed GetGameCoordinator symbol; no guessed ABI'},indent=2))
print('PREFLIGHT_PASSED ELF32/API/source hashes verified; isolated payload prepared')
