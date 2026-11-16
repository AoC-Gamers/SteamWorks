"""Evidence-tool tests. Generated fixtures are not Steam ABI acceptance."""
from __future__ import annotations
import copy
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

def load(name: str, filename: str):
    spec = importlib.util.spec_from_file_location(name, ROOT / 'scripts' / filename)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

collector = load('collector', 'inspect-steamworks-module.py')
validator = load('validator', 'validate-runtime-evidence.py')
PROFILE_DIR = ROOT / 'docs/runtime-compatibility/p4'


class EvidenceTests(unittest.TestCase):
    def setUp(self):
        self.profile = json.loads((PROFILE_DIR / 'linux-x86-candidate.json').read_text())

    def test_documentary_candidate_is_consistent_and_disabled(self):
        validator.validate(self.profile, PROFILE_DIR)
        self.assertFalse(self.profile['enabled'])

    def test_no_documentary_activation_or_self_approval(self):
        for key, value in [('enabled', True), ('documentation_only', False),
                           ('not_consumed_by_current_extension', False),
                           ('verification_status', 'verified'), ('approved_by', 'guessed'),
                           ('approved_at', '2026-11-14')]:
            with self.subTest(key=key):
                changed = copy.deepcopy(self.profile)
                changed[key] = value
                with self.assertRaises(ValueError):
                    validator.validate(changed, PROFILE_DIR)

    def test_sdk_identity_cannot_stand_in_for_runtime(self):
        for name in ('engine', 'steam-runtime'):
            for key, value in [('sha256', self.profile['modules'][2]['sha256']),
                               ('loaded_module_confirmed', True)]:
                with self.subTest(module=name, field=key):
                    changed = copy.deepcopy(self.profile)
                    next(item for item in changed['modules'] if item['module_id'] == name)[key] = value
                    with self.assertRaises(ValueError):
                        validator.validate(changed, PROFILE_DIR)

    def test_unverified_entrypoints_stay_unavailable(self):
        for key, value in [('enabled', True), ('provider_module_id', 'sdk-reference'),
                           ('verified_prototype', 'guessed'), ('trampoline_validation_evidence', 'mock'),
                           ('pattern_bytes', '00 90'), ('pattern_mask', 'xx'), ('vtable_offset', 0)]:
            with self.subTest(field=key):
                changed = copy.deepcopy(self.profile)
                changed['entrypoints'][0][key] = value
                with self.assertRaises(ValueError):
                    validator.validate(changed, PROFILE_DIR)

    def test_module_metadata_and_evidence_hash_are_bound(self):
        for key, value in [('architecture', 'x86_64'), ('format', 'PE'), ('sha256', '0' * 64),
                           ('size_bytes', 1), ('build_id', 'fake'), ('evidence_sha256', '0' * 64),
                           ('evidence_file', '../../p0/sdk-toolchain.json')]:
            with self.subTest(field=key):
                changed = copy.deepcopy(self.profile)
                changed['modules'][2][key] = value
                with self.assertRaises(ValueError):
                    validator.validate(changed, PROFILE_DIR)

    def test_minimal_unique_modules_and_entrypoints(self):
        for target in ('modules', 'entrypoints'):
            changed = copy.deepcopy(self.profile)
            changed[target].append(copy.deepcopy(changed[target][0]))
            with self.assertRaises(ValueError):
                validator.validate(changed, PROFILE_DIR)
        changed = copy.deepcopy(self.profile)
        changed['blocking_dependencies'] = []
        with self.assertRaises(ValueError):
            validator.validate(changed, PROFILE_DIR)

    def test_real_sdk_reference_exports_and_identity(self):
        sdk = Path(os.environ.get('STEAMWORKS_SDK_DIR', ROOT / 'sdk_158a'))
        info = collector.inspect(sdk / 'redistributable_bin/linux32/libsteam_api.so', 'test SDK', 'sdk-reference')
        reference = json.loads((PROFILE_DIR / 'modules/sdk_158a-linux32.json').read_text())
        for key in ('sha256', 'size_bytes', 'build_id', 'format', 'architecture', 'exports'):
            self.assertEqual(info[key], reference[key], key)
        self.assertFalse(info['loaded_module_confirmed'])
        self.assertFalse(info['abi_verified'])
        self.assertFalse(info['initialization_route_verified'])
        self.assertEqual(len(info['exports']['SteamGameServer_InitSafe']), 1)
        self.assertEqual(info['exports']['SteamInternal_GameServer_Init'], [])
        self.assertEqual(len(info['exports']['SteamInternal_GameServer_Init_V2']), 1)

    def test_reference_versions_do_not_imply_same_init_route(self):
        old = json.loads((PROFILE_DIR / 'modules/sdk_155-linux32.json').read_text())
        new = json.loads((PROFILE_DIR / 'modules/sdk_158a-linux32.json').read_text())
        self.assertEqual(len(old['exports']['SteamInternal_GameServer_Init']), 1)
        self.assertEqual(new['exports']['SteamInternal_GameServer_Init'], [])
        self.assertIn('SteamInternal_GameServer_Init@plt', old['static_direct_call_labels']['SteamGameServer_InitSafe'])
        self.assertIn('SteamInternal_GameServer_Init_V2', new['static_direct_call_labels']['SteamGameServer_InitSafe'])
        self.assertNotEqual(old['sha256'], new['sha256'])

    def test_pe_export_presence_is_not_windows_acceptance(self):
        info = json.loads((PROFILE_DIR / 'modules/sdk_158a-win32.json').read_text())
        self.assertEqual(info['format'], 'PE')
        self.assertEqual(info['architecture'], 'x86')
        self.assertFalse(info['abi_verified'])
        self.assertFalse(info['loaded_module_confirmed'])
        self.assertEqual(len(info['exports']['SteamGameServer_InitSafe']), 1)
        self.assertEqual(list(PROFILE_DIR.glob('*windows*')), [])

    def test_malformed_or_unsupported_binary_fails_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'module with spaces.so'
            for data in (b'', b'not a module', b'MZ' + b'\0' * 62,
                         b'\x7fELF' + bytes((1, 2)) + b'\0' * 60):
                with self.subTest(data=data[:6]):
                    path.write_bytes(data)
                    with self.assertRaises(ValueError):
                        collector.inspect(path, 'synthetic corrupt file', 'sdk-reference')

    @unittest.skipUnless(sys.platform.startswith('linux'), '/proc mapping check is Linux only')
    def test_loaded_mapping_requires_exact_file_device_and_inode(self):
        info = collector.inspect(Path(sys.executable), 'test Python executable, not game', 'engine', os.getpid())
        self.assertTrue(info['loaded_module_confirmed'])
        self.assertFalse(info['abi_verified'])
        self.assertFalse(info['enabled'])
        with tempfile.NamedTemporaryFile() as handle:
            self.assertFalse(collector.loaded_in_process(os.getpid(), os.fstat(handle.fileno())))

    def test_bad_cli_input_does_not_write_evidence(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / 'not-created.json'
            result = subprocess.run([sys.executable, str(ROOT / 'scripts/inspect-steamworks-module.py'),
                                     str(Path(directory) / 'missing.so'), '--origin', 'fixture',
                                     '--role', 'sdk-reference', '--output', str(output)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 2)
            self.assertFalse(output.exists())

    def test_profile_validation_cli_rejects_activation(self):
        with tempfile.TemporaryDirectory() as directory:
            profile = Path(directory) / 'profile.json'
            self.profile['enabled'] = True
            profile.write_text(json.dumps(self.profile))
            result = subprocess.run([sys.executable, str(ROOT / 'scripts/validate-runtime-evidence.py'), str(profile)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 2)
            self.assertIn('cannot enable code', result.stderr)

    def test_selected_runtime_profile_is_coherent(self):
        profile = json.loads((PROFILE_DIR / 'competitive-dev-linux-x86.json').read_text())
        validator.validate(profile, PROFILE_DIR)
        header = (ROOT / 'extension/core/swlifecycleprofile.h').read_text()
        for module in profile['modules']:
            self.assertIn(module['build_id'], header)
        self.assertIn(profile['profile_id'], header)
        self.assertTrue(profile['operational_selector']['enabled_in_product'])
        self.assertFalse(profile['enabled'])  # JSON is documentary; selector is code.

    def test_selected_runtime_profile_rejects_corruption(self):
        base = json.loads((PROFILE_DIR / 'competitive-dev-linux-x86.json').read_text())
        for change in ('architecture', 'hash', 'rva', 'relocation', 'pattern', 'approval', 'trace', 'prefix', 'prototype'):
            with self.subTest(change=change):
                profile = copy.deepcopy(base)
                if change == 'architecture': profile['environment']['architecture'] = 'x86_64'
                if change == 'hash': profile['modules'][0]['sha256'] = '0' * 64
                if change == 'rva': profile['entrypoints'][0]['rva'] += 1
                if change == 'relocation': profile['entrypoints'][0]['engine_relocation_offset'] += 1
                if change == 'pattern': profile['entrypoints'][0]['pattern_bytes'] = '00 90'
                if change == 'approval': profile['approved_by'] = 'invented'
                if change == 'trace': profile['trace_sha256'] = '0' * 64
                if change == 'prefix': profile['entrypoints'][0]['original_prefix'] = '00 90'
                if change == 'prototype': profile['entrypoints'][0]['verified_prototype']['arguments'].append('extra argument')
                with self.assertRaises(ValueError): validator.validate(profile, PROFILE_DIR)

    def test_removed_memory_helpers_have_no_product_consumer(self):
        self.assertFalse((ROOT / 'extension/core/swmemutils.cpp').exists())
        self.assertFalse((ROOT / 'extension/core/swmemutils.h').exists())
        for path in (ROOT / 'extension').rglob('*'):
            if path.suffix in ('.cpp', '.h') or path.name == 'AMBuilder':
                self.assertNotIn('SteamWorksMemUtils', path.read_text(), str(path))

    def test_packaging_keeps_profiles_out_of_gamedata(self):
        # Layout-only fixture; it is not an extension build or package acceptance.
        with tempfile.TemporaryDirectory() as directory:
            build = Path(directory) / 'build'
            (build / 'addons/sourcemod').mkdir(parents=True)
            artifact = Path(directory) / 'artifact'
            result = subprocess.run([sys.executable, str(ROOT / 'scripts/stage-artifact.py'),
                                     str(ROOT), str(build), str(artifact)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue((artifact / 'docs/runtime-compatibility/p4/linux-x86-candidate.json').exists())
            self.assertFalse((artifact / 'addons/sourcemod/gamedata').exists())
            self.assertEqual(list(artifact.rglob('libsteam_api.so')), [])
            self.assertEqual(list(artifact.rglob('steam_api.dll')), [])
            self.assertEqual(list((artifact / 'addons').rglob('*profile*.json')), [])

if __name__ == '__main__':
    unittest.main(verbosity=2)
