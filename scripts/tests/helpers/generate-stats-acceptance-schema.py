#!/usr/bin/env python3
"""Generate a public-key-only SourcePawn fixture from the checked-in AppID 550 schema."""
from pathlib import Path
import argparse,hashlib,json
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('output',type=Path)
a=p.parse_args()
source=Path(__file__).resolve().parents[3]/'tests/fixtures/stats/GetSchemaForGame_550.json'
game=json.loads(source.read_text())['game']['availableGameStats']
keys=[s['name'] for s in game['stats']]
assert len(keys)==len(set(keys)) and all(k.isascii() for k in keys)
missing='__steamworks_accept_missing_stat_p5_r4__'
assert missing not in keys and missing not in [s['name'] for s in game['achievements']]
a.output.parent.mkdir(parents=True,exist_ok=True)
a.output.write_text('// Generated from GetSchemaForGame_550.json SHA256 '+hashlib.sha256(source.read_bytes()).hexdigest()+'\n#define MISSING_STAT '+json.dumps(missing)+'\nchar g_StatKeys[][] =\n{\n'+',\n'.join('\t'+json.dumps(k) for k in keys)+'\n};\n')
print(f'Generated {len(keys)} stat keys; missing fixture absent from stats and achievements.')
