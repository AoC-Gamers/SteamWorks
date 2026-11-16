from pathlib import Path
import json,re,sys,hashlib
x=json.load(sys.stdin);pid=x['pid'];maps=Path('/proc/'+str(pid)+'/maps').read_text().splitlines()
def owner(address):
 match=[l.split(maxsplit=5) for l in maps if int(l.split('-')[0],16)<=address<int(l.split()[0].split('-')[1],16) and 'x' in l.split()[1]]
 assert len(match)==1 and len(match[0])==6
 p=Path(match[0][5]);return {'module':p.name,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}
entries={}
for line in x['trace']:
 m=re.search(r'entry=(\w+) address=([a-f0-9]+) opcode=([a-f0-9]+) target=([a-f0-9]+)',line)
 if m:entries[m[1]]={'address':int(m[2],16),'target':int(m[4],16),'opcode':m[3]}
code=next(l for l in x['trace'] if 'code=SteamInit ' in l)
wire=bytes.fromhex(code.split('bytes=')[1]);assert wire[0]==0xe9
consumer=entries['SteamInit']['target']+5+int.from_bytes(wire[1:5],'little',signed=True)
result={'PID':pid,'engine_init_CALL_owner':owner(entries['EngineInitCall']['target']),'shutdown_detour_owner':owner(entries['SteamShutdown']['target']),'SDK_init_bridge_handler_owner':owner(consumer)}
assert entries['EngineInitCall']['opcode']=='e8'
assert result['engine_init_CALL_owner']['module']=='steamworks.ext.so' and result['shutdown_detour_owner']['module']=='steamworks.ext.so'
assert result['SDK_init_bridge_handler_owner']['module']=='sourcetvsupport.ext.2.l4d2.so'
result['coexistence_passed']=True
print(json.dumps(result,indent=2))
