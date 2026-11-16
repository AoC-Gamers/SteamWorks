#!/usr/bin/env python3
"""Read-only evidence collector. Does not load a library, start Steam or approve ABI."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess

ENTRYPOINTS = (
    'SteamGameServer_InitSafe', 'SteamInternal_GameServer_Init',
    'SteamInternal_GameServer_Init_V2', 'SteamGameServer_Shutdown',
    'SteamInternal_CreateInterface', 'SteamGameServerInternal_CreateInterface',
    'SteamGameServer_GetHSteamUser', 'SteamGameServer_GetHSteamPipe',
    'g_pSteamClientGameServer',
)

def run(*args: str) -> str:
    result = subprocess.run(args, capture_output=True, text=True, check=False, env={**os.environ, 'LC_ALL': 'C'})
    if result.returncode:
        raise ValueError(f'{args[0]} failed: {result.stderr.strip()}')
    return result.stdout


def loaded_in_process(pid: int | None, stat: os.stat_result) -> bool:
    if pid is None:
        return False
    for line in Path(f'/proc/{pid}/maps').read_text().splitlines():
        fields = line.split(maxsplit=5)
        if len(fields) < 5:
            continue
        major, minor = (int(part, 16) for part in fields[3].split(':'))
        if int(fields[4]) == stat.st_ino and os.makedev(major, minor) == stat.st_dev:
            return True
    return False


def inspect(path: Path, origin: str, role: str, pid: int | None = None) -> dict:
    path = path.resolve(strict=True)
    before = path.stat()
    data = path.read_bytes()
    if before.st_size != len(data):
        raise ValueError('Module changed while reading')
    identity = {'format': None, 'architecture': None, 'size_bytes': len(data),
                'sha256': hashlib.sha256(data).hexdigest(), 'build_id': None}
    exports: dict[str, list[dict]] = {name: [] for name in ENTRYPOINTS}
    imports: set[str] = set()
    relocations: list[dict] = []
    if data[:4] == b'\x7fELF':
        if len(data) < 52 or data[4] not in (1, 2) or data[5] != 1:
            raise ValueError('Unsupported or truncated ELF encoding')
        machine = struct.unpack_from('<H', data, 18)[0]
        architecture = {(1, 3): 'x86', (2, 62): 'x86_64'}.get((data[4], machine))
        if architecture is None:
            raise ValueError('Unsupported ELF architecture')
        identity.update(format='ELF', architecture=architecture)
        notes = run('readelf', '-n', str(path))
        match = re.search(r'Build ID: ([0-9a-f]+)', notes)
        identity['build_id'] = match.group(1) if match else None
        symbols = run('readelf', '--dyn-syms', '-W', str(path))
        for line in symbols.splitlines():
            fields = line.split()
            if len(fields) < 8 or not fields[0].endswith(':') or not fields[0][:-1].isdigit():
                continue
            _, value, size, kind, binding, visibility, section, name = fields[:8]
            name = name.split('@')[0]
            if section == 'UND':
                imports.add(name)
            elif name in exports and binding in ('GLOBAL', 'WEAK') and visibility in ('DEFAULT', 'PROTECTED'):
                exports[name].append({'rva': int(value, 16), 'size_bytes': int(size), 'kind': kind})
        for line in run('readelf', '-rW', str(path)).splitlines():
            fields = line.split()
            if len(fields) >= 5 and fields[2].startswith('R_'):
                name = fields[4].split('@')[0]
                if name in ENTRYPOINTS:
                    relocations.append({'offset': int(fields[0], 16), 'type': fields[2], 'symbol': name})
    elif data[:2] == b'MZ':
        if len(data) < 64:
            raise ValueError('Truncated PE DOS header')
        offset = struct.unpack_from('<I', data, 60)[0]
        if offset > len(data) - 24 or data[offset:offset + 4] != b'PE\0\0':
            raise ValueError('Invalid PE header')
        machine = struct.unpack_from('<H', data, offset + 4)[0]
        architecture = {0x14c: 'x86', 0x8664: 'x86_64'}.get(machine)
        if architecture is None:
            raise ValueError('Unsupported PE architecture')
        identity.update(format='PE', architecture=architecture)
        # Names alone establish presence, not calling convention or export ordinal ABI.
        details = run('objdump', '-p', str(path))
        in_names = False
        for line in details.splitlines():
            if '[Ordinal/Name Pointer] Table' in line:
                in_names = True
                continue
            if in_names:
                match = re.match(r'\s*\[\s*\d+\]\s+(\S+)\s*$', line)
                if match and match.group(1) in exports:
                    exports[match.group(1)].append({'rva': None, 'size_bytes': None, 'kind': 'export_name_only'})
    else:
        raise ValueError('Expected ELF or PE module')
    after = path.stat()
    if (before.st_dev, before.st_ino, before.st_size, before.st_mtime_ns) != (after.st_dev, after.st_ino, after.st_size, after.st_mtime_ns):
        raise ValueError('Module identity changed during inspection')
    return {'schema_version': 1, 'documentation_only': True, 'enabled': False,
            'file_name': path.name, 'input_path': str(path), 'origin': origin, 'role': role,
            **identity, 'pid_observed': pid,
            'loaded_module_confirmed': loaded_in_process(pid, after),
            'exports': exports, 'imports_of_interest': sorted(imports.intersection(ENTRYPOINTS)),
            'relocations_of_interest': relocations,
            'abi_verified': False, 'initialization_route_verified': False,
            'notes': ['SDK files are reference/link dependencies unless a selected process maps this exact device/inode.',
                      'Export/import/relocation presence is not proof of an executed call path or ABI.',
                      'Loaded status verifies process maps device/inode; it is not a hash of mapped memory.']}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('module', type=Path)
    parser.add_argument('--origin', required=True)
    parser.add_argument('--role', choices=('sdk-reference', 'engine', 'steam-runtime'), required=True)
    parser.add_argument('--pid', type=int)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    try:
        result = inspect(args.module, args.origin, args.role, args.pid)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(result, indent=2) + '\n')
    except (OSError, ValueError) as error:
        parser.exit(2, f'Inspection failed: {error}\n')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
