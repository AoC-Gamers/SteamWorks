#!/usr/bin/env python3
"""Require the exact Steamworks SDK 1.55 headers and x86 link libraries."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def validate(sdk, lock_path=None):
    sdk = Path(sdk).resolve()
    lock = json.loads(Path(lock_path or ROOT / 'scripts/build/steamworks-sdk.lock.json').read_text())
    if lock.get('schema_version') != 1 or lock.get('sdk_version') != '1.55':
        raise ValueError('Unsupported SDK lock; only Steamworks 1.55 is permitted')
    expected = lock['files']
    headers = {p.relative_to(sdk).as_posix() for p in (sdk / 'public/steam').glob('*.h')}
    if headers != {p for p in expected if p.startswith('public/steam/')}:
        raise ValueError('SDK 1.55 header set differs from the pinned release')
    for relative, sha in expected.items():
        path = sdk / relative
        if not path.is_file():
            raise ValueError('SDK 1.55 file missing or changed: ' + relative)
        data = path.read_bytes()
        if relative.startswith('public/steam/'):
            data = data.replace(b'\r\n', b'\n')
        if hashlib.sha256(data).hexdigest() != sha:
            raise ValueError('SDK 1.55 file missing or changed: ' + relative)
    return lock


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sdk', type=Path)
    args = parser.parse_args()
    try:
        lock = validate(args.sdk)
    except (OSError, ValueError, KeyError) as error:
        parser.exit(2, str(error) + '\n')
    print('SDK_155_VALIDATION_OK ' + str(len(lock['files'])) + ' pinned files')


if __name__ == '__main__':
    main()
