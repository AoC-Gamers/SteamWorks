"""Exact SDK selection and rejection of mutated or alternative SDK content."""
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location('sdk_guard', ROOT / 'scripts/build/validate-steamworks-sdk.py')
guard = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(guard)
SDK = Path(os.environ.get('STEAMWORKS_SDK_DIR', ROOT / 'sdk_155'))


class SDKSelectionTests(unittest.TestCase):
    def test_selected_release_and_all_link_inputs(self):
        lock = guard.validate(SDK)
        self.assertEqual(lock['sdk_version'], '1.55')
        self.assertEqual(lock['directory_name'], 'sdk_155')
        self.assertIn('redistributable_bin/linux32/libsteam_api.so', lock['files'])
        self.assertIn('redistributable_bin/steam_api.lib', lock['files'])

    def test_changed_headers_missing_libraries_and_added_headers_are_rejected(self):
        lock = guard.validate(SDK)
        with tempfile.TemporaryDirectory(prefix='sdk selection ') as temp:
            copy = Path(temp) / 'renamed SDK'
            for name in lock['files']:
                path = copy / name
                path.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(SDK / name, path)
            guard.validate(copy)  # Path spelling does not change release identity.
            header = copy / 'public/steam/isteamgameserver.h'
            text = header.read_bytes().replace(b'\r\n', b'\n')
            header.write_bytes(text)
            guard.validate(copy)
            header.write_bytes(text.replace(b'\n', b'\r\n'))
            guard.validate(copy)  # Git checkout line endings do not change this text ABI.

            for name in ('public/steam/isteamgameserver.h', 'redistributable_bin/linux32/libsteam_api.so', 'redistributable_bin/steam_api.lib'):
                path = copy / name
                original = path.read_bytes()
                for changed in (None, original + b'changed'):
                    if changed is None: path.unlink()
                    else: path.write_bytes(changed)
                    with self.assertRaisesRegex(ValueError, 'SDK 1.55'):
                        guard.validate(copy)
                path.write_bytes(original)
            extra = copy / 'public/steam/unreviewed.h'
            extra.write_text('unreviewed release')
            with self.assertRaisesRegex(ValueError, 'header set'):
                guard.validate(copy)

    def test_cli_uses_relocated_lock_from_sources_without_git(self):
        with tempfile.TemporaryDirectory(prefix='sdk sources with spaces ') as temp:
            root = Path(temp)
            tool = root / 'scripts/build/validate-steamworks-sdk.py'
            tool.parent.mkdir(parents=True)
            shutil.copy2(ROOT / 'scripts/build/validate-steamworks-sdk.py', tool)
            lock = root / 'scripts/build/steamworks-sdk.lock.json'
            shutil.copy2(ROOT / 'scripts/build/steamworks-sdk.lock.json', lock)
            original = lock.read_bytes()
            legacy = root / 'steamworks-sdk.lock.json'
            legacy.write_text('{"sdk_version":"1.55a"}')
            command = [sys.executable, str(tool), str(SDK.resolve())]
            accepted = subprocess.run(command, cwd=temp, capture_output=True, text=True)
            self.assertEqual(accepted.returncode, 0, accepted.stderr)
            self.assertIn('47 pinned files', accepted.stdout)
            lock.unlink()
            legacy.write_bytes(original)
            rejected = subprocess.run(command, cwd=temp, capture_output=True, text=True)
            self.assertEqual(rejected.returncode, 2)
            self.assertIn('steamworks-sdk.lock.json', rejected.stderr)

    def test_unknown_lock_version_and_missing_sdk_fail_cli(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'lock.json'
            lock = json.loads((ROOT / 'scripts/build/steamworks-sdk.lock.json').read_text())
            lock['sdk_version'] = '1.55a'
            path.write_text(json.dumps(lock))
            with self.assertRaisesRegex(ValueError, 'only Steamworks 1.55'):
                guard.validate(SDK, path)
            result = subprocess.run([sys.executable, str(ROOT / 'scripts/build/validate-steamworks-sdk.py'), str(Path(temp) / 'missing')], capture_output=True, text=True)
            self.assertEqual(result.returncode, 2)
            self.assertIn('SDK 1.55', result.stderr)


if __name__ == '__main__':
    unittest.main(verbosity=2)
