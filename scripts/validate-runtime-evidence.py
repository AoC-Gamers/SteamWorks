#!/usr/bin/env python3
"""Validate documentary evidence; no profile in this phase can enable the extension."""
from __future__ import annotations
import argparse
import hashlib
import json
import re
from pathlib import Path


def validate(profile: dict, directory: Path) -> None:
    if not isinstance(profile, dict):
        raise ValueError('Expected an object profile')
    if profile.get('schema_version') != 1 or profile.get('documentation_only') is not True:
        raise ValueError('Expected documentary profile schema 1')
    if profile.get('enabled') is not False or profile.get('not_consumed_by_current_extension') is not True:
        raise ValueError('No operational binary profile is approved; documentary files cannot enable code')
    if profile.get('verification_status') == 'validated_local':
        validate_local(profile, directory)
        return
    if profile.get('verification_status') != 'blocked_missing_runtime':
        raise ValueError('Runtime acceptance status must remain blocked')
    if profile.get('approved_by') is not None or profile.get('approved_at') is not None:
        raise ValueError('No approval has been obtained')
    modules = profile.get('modules', [])
    if not isinstance(modules, list) or not all(isinstance(item, dict) for item in modules):
        raise ValueError('Expected module records')
    ids = [item.get('module_id') for item in modules]
    if len(ids) != len(set(ids)) or not {'engine', 'steam-runtime', 'sdk-reference'}.issubset(ids):
        raise ValueError('Unique engine, Steam runtime and SDK reference records required')
    by_id = {item['module_id']: item for item in modules}
    for name in ('engine', 'steam-runtime'):
        item = by_id[name]
        if item.get('sha256') is not None or item.get('loaded_module_confirmed') is not False:
            raise ValueError('Unobserved runtime must not inherit SDK identity or loaded status')
    for item in modules:
        reference = item.get('evidence_file')
        if reference is None:
            continue
        path = (directory / reference).resolve()
        if not path.is_relative_to(directory.resolve()):
            raise ValueError('Evidence reference escapes its directory')
        data = json.loads(path.read_text())
        for key in ('sha256', 'architecture', 'format', 'size_bytes', 'build_id'):
            if item.get(key) != data.get(key):
                raise ValueError(f'Module {item["module_id"]}: inconsistent {key}')
        if hashlib.sha256(path.read_bytes()).hexdigest() != item.get('evidence_sha256'):
            raise ValueError('Evidence document hash changed')
        if data.get('role') != 'sdk-reference' or data.get('abi_verified') is not False:
            raise ValueError('Expected unapproved SDK reference evidence')
    entries = profile.get('entrypoints', [])
    if not isinstance(entries, list) or not all(isinstance(entry, dict) for entry in entries):
        raise ValueError('Expected entrypoint records')
    names = [entry.get('name') for entry in entries]
    if len(names) != len(set(names)) or set(names) != {'SteamInternal_GameServer_Init', 'SteamGameServer_Shutdown'}:
        raise ValueError('Only the minimal observed candidate init/shutdown pair belongs in this profile')
    for entry in entries:
        if entry.get('enabled') is not False or entry.get('provider_module_id') is not None:
            raise ValueError('Unobserved provider cannot enable an entrypoint')
        if entry.get('verified_prototype') is not None or entry.get('trampoline_validation_evidence') is not None:
            raise ValueError('ABI and trampoline must not be inferred from export presence')
        if any(entry.get(key) is not None for key in ('pattern_bytes', 'pattern_mask', 'vtable_offset')):
            raise ValueError('No pattern or vtable offset is approved')
    if not profile.get('blocking_dependencies'):
        raise ValueError('Missing concrete runtime acceptance dependencies')


def validate_local(profile: dict, directory: Path) -> None:
    if profile.get('profile_id') != 'l4d2-23990100-linux-x86':
        raise ValueError('Unknown operational profile identity')
    if profile.get('approved_by') is not None or profile.get('approved_at') is not None:
        raise ValueError('Local validation is not a release approval')
    environment = profile.get('environment', {})
    if environment.get('platform') != 'linux' or environment.get('architecture') != 'x86':
        raise ValueError('Profile is scoped to Linux x86')
    records = profile.get('modules', [])
    if len(records) != 2 or {item.get('module_id') for item in records} != {'engine', 'steam-runtime'}:
        raise ValueError('Exactly one engine/provider pair required')
    modules = {}
    for item in records:
        path = (directory / item['evidence_file']).resolve()
        if not path.is_relative_to(directory.resolve()):
            raise ValueError('Evidence reference escapes directory')
        data = json.loads(path.read_text())
        for key in ('sha256', 'build_id', 'format', 'architecture', 'size_bytes'):
            if data.get(key) != item.get(key):
                raise ValueError('Inconsistent module evidence')
        if data.get('loaded_module_confirmed') is not True or not data.get('copy_hash_matches_mapped_file'):
            raise ValueError('Mapped provider identity required')
        if hashlib.sha256(path.read_bytes()).hexdigest() != item.get('evidence_sha256'):
            raise ValueError('Evidence document hash changed')
        modules[item['module_id']] = data
    header = (Path(__file__).resolve().parents[1] / 'extension/core/swlifecycleprofile.h').read_text()
    for module, variable in (('engine', 'LifecycleEngineBuildId'), ('steam-runtime', 'LifecycleSteamBuildId')):
        match = re.search(variable + r'\s*=\s*"([^"]+)"', header)
        if match is None or match.group(1) != modules[module]['build_id']:
            raise ValueError('Compiled identity differs from evidence')
    rvas = re.search(r'const uintptr_t rva = entry == LifecycleEntry::Init \? (0x[0-9a-f]+) : (0x[0-9a-f]+)', header)
    if rvas is None:
        raise ValueError('Compiled RVA declarations unavailable')
    entries = profile.get('entrypoints', [])
    if {item.get('name') for item in entries} != {'SteamInternal_GameServer_Init', 'SteamGameServer_Shutdown'} or len(entries) != 2:
        raise ValueError('Minimal init/shutdown pair required')
    for item in entries:
        if item.get('provider_module_id') != 'steam-runtime' or item.get('resolution_method') != 'export':
            raise ValueError('Only measured provider exports accepted')
        if any(item.get(key) is not None for key in ('pattern_bytes', 'pattern_mask', 'vtable_offset')):
            raise ValueError('No pattern or vtable index accepted')
        index = 0 if item['name'] == 'SteamInternal_GameServer_Init' else 1
        array_name = 'init' if index == 0 else 'shutdown'
        prefix = re.search(r'unsigned char ' + array_name + r'\[\] = \{([^}]+)\}', header)
        expected = bytes(int(x.strip(), 16) for x in prefix.group(1).split(',')) if prefix else b''
        if bytes.fromhex(item['original_prefix']) != expected or item['minimum_relocated_bytes'] != len(expected) or item['rva'] != int(rvas.group(index + 1), 16):
            raise ValueError('Compiled entrypoint differs from evidence')
        proto = item.get('verified_prototype', {})
        if len(proto.get('arguments', [])) != (6 if index == 0 else 0) or proto.get('return') != ('bool in AL' if index == 0 else 'void'):
            raise ValueError('Legacy entrypoint prototype differs')
        exports = modules['steam-runtime']['exports'][item['name']]
        if len(exports) != 1 or exports[0]['rva'] != item['rva']:
            raise ValueError('Export identity/RVA differs')
        relocation = {'offset': item['engine_relocation_offset'], 'type': 'R_386_PC32', 'symbol': item['name']}
        if relocation not in modules['engine']['relocations_of_interest']:
            raise ValueError('Engine call relocation is not present')
        if not item.get('verified_prototype') or not item.get('trampoline_validation_evidence'):
            raise ValueError('Prototype and trampoline evidence required')
    trace_path = (directory / profile['trace_file']).resolve()
    if not trace_path.is_relative_to(directory.resolve()) or hashlib.sha256(trace_path.read_bytes()).hexdigest() != profile.get('trace_sha256'):
        raise ValueError('Lifecycle trace identity differs')
    trace = json.loads(trace_path.read_text())
    if trace['round1']['init_successes'] < 3 or trace['round1']['shutdowns'] < 3 or not trace['round1']['unload_reload_confirmed'] or trace['round2']['errors']:
        raise ValueError('Incomplete or failed observed lifecycle')


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('profile', type=Path)
    args = parser.parse_args()
    try:
        validate(json.loads(args.profile.read_text()), args.profile.parent)
    except (OSError, ValueError, KeyError, TypeError) as error:
        parser.exit(2, f'Evidence rejected: {error}\n')
    print('DOCUMENTARY_EVIDENCE_OK; JSON cannot activate code paths')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
