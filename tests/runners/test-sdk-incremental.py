#!/usr/bin/env python3
"""Exercise the production AMBuild SDK guard after an initial x86 build.

Requires real AMBuild, SourceMod/Metamod/HL2SDK build dependencies and SDK 1.55.
Mutates only a private SDK copy; never starts a server or edits dependencies.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
AMBUILD = [sys.executable, '-c', 'from ambuild2 import run; run.cli_run()']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--sdk', type=Path, default=os.getenv('STEAMWORKS_SDK_DIR', ROOT / 'sdk_155'))
    parser.add_argument('--sm-path', type=Path, required=True)
    parser.add_argument('--mms-path', type=Path, required=True)
    parser.add_argument('--hl2sdk-root', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    original_sdk = args.sdk.resolve()
    subprocess.run([sys.executable, str(ROOT / 'scripts/build/validate-steamworks-sdk.py'), str(original_sdk)], check=True)
    lock = json.loads((ROOT / 'steamworks-sdk.lock.json').read_text())
    sdk = output / 'private-sdk'
    for name in lock['files']:
        target = sdk / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(original_sdk / name, target)
    build = output / 'build'
    build.mkdir()
    results = []

    def run(name, command, succeeds=True):
        with (output / (name + '.log')).open('w') as log:
            result = subprocess.run(command, cwd=build, stdout=log, stderr=subprocess.STDOUT)
        results.append({'case': name, 'exit_code': result.returncode})
        if succeeds and result.returncode:
            raise RuntimeError(name + ' failed; see log')
        if not succeeds:
            text = (output / (name + '.log')).read_text()
            if result.returncode == 0 or 'SDK 1.55' not in text:
                raise RuntimeError(name + ' did not reject the SDK mutation for its intended reason')
            if 'Build succeeded.' in text:
                raise RuntimeError(name + ' generated a build after rejecting the SDK')

    run('configure', [sys.executable, str(ROOT / 'configure.py'),
                     '--steamworks-path', str(sdk), '--sm-path', str(args.sm_path.resolve()),
                     '--mms-path', str(args.mms_path.resolve()), '--hl2sdk-root', str(args.hl2sdk_root.resolve()),
                     '--sdks', 'l4d2', '--target', 'x86', '--enable-optimize'])
    run('initial-build', AMBUILD)
    extension = build / 'package/addons/sourcemod/extensions/steamworks.ext.2.l4d2.so'
    if extension.read_bytes()[:7] != b'\x7fELF\x01\x01\x01':
        raise RuntimeError('Expected an actual x86 build')

    def stamp(path):
        # Exercise watchers even on filesystems with coarse timestamp resolution.
        future = time.time() + 2
        os.utime(path, (future, future))

    for name, relative in [('changed-header', 'public/steam/isteamgameserver.h'),
                           ('changed-unused-library', 'redistributable_bin/steam_api.lib'),
                           ('missing-header', 'public/steam/isteamgameserver.h'),
                           ('extra-header', 'public/steam/unreviewed.h')]:
        path = sdk / relative
        original = path.read_bytes() if path.exists() else None
        before = (hashlib.sha256(extension.read_bytes()).hexdigest(), extension.stat().st_mtime_ns)
        if name == 'missing-header':
            path.unlink()
        else:
            path.write_bytes((original or b'') + b'\n// deterministic SDK pin mutation\n')
            stamp(path)
        if name in ('missing-header', 'extra-header'):
            stamp(sdk / 'public/steam')
        run(name, AMBUILD, succeeds=False)
        after = (hashlib.sha256(extension.read_bytes()).hexdigest(), extension.stat().st_mtime_ns)
        if after != before:
            raise RuntimeError(name + ' rewrote the product despite the invalid SDK')
        if original is None:
            path.unlink()
        else:
            shutil.copy2(original_sdk / relative, path)
            stamp(path)
        if name in ('missing-header', 'extra-header'):
            stamp(sdk / 'public/steam')
        run(name + '-restored', AMBUILD)

    # Reject changes rather than silently accepting them, including the original SDK.
    subprocess.run([sys.executable, str(ROOT / 'scripts/build/validate-steamworks-sdk.py'), str(original_sdk)], check=True)
    (output / 'results.json').write_text(json.dumps({'status': 'passed', 'platform': 'linux-x86',
        'original_SDK_untouched': True, 'cases': results}, indent=2) + '\n')
    print('SDK_INCREMENTAL_OK four rejected mutations and four restored builds')


if __name__ == '__main__':
    main()
