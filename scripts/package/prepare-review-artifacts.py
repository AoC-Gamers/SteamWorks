#!/usr/bin/env python3
"""Prepare local Linux x86 candidate/rollback archives; never access a server.

Baseline is a previously captured sourcemod root. Only SteamWorks-owned files
are read. Configurations and gamedata absence are recorded rather than invented.
"""
import argparse
import hashlib
import json
import re
from datetime import datetime, timezone
from pathlib import Path
import tempfile
import zipfile

import importlib.util
_contract_spec = importlib.util.spec_from_file_location("artifact_contract", Path(__file__).with_name("artifact_contract.py"))
_contract = importlib.util.module_from_spec(_contract_spec)
_contract_spec.loader.exec_module(_contract)

LEGACY_BINARY = "extensions/steamworks.ext.so"
OWNED = ("extensions/steamworks.ext.2.l4d2.so", "scripting/include/steamworks.inc",
         "gamedata/steamworks.txt", LEGACY_BINARY)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def linux_x86(data):
    # ELF32, little endian, ET_DYN, EM_386.
    return (len(data) >= 52 and data[:7] == b"\x7fELF\x01\x01\x01"
            and data[16:20] == b"\x03\x00\x03\x00")


def archive(path, files):
    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED) as output:
        for name, data in sorted(files.items()):
            info = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            output.writestr(info, data)
    with zipfile.ZipFile(path) as check:
        if check.testzip() or set(check.namelist()) != set(files):
            raise ValueError("Archive verification failed")
        for name, data in files.items():
            if check.read(name) != data:
                raise ValueError("Archive content differs: " + name)


def prepare(candidate, baseline, output, label="p5"):
    if len(label) > 64 or not re.fullmatch(r"[a-z0-9]+(?:-[a-z0-9]+)*", label):
        raise ValueError("Invalid artifact label")
    candidate, baseline, output = map(Path, (candidate, baseline, output))
    if output.exists():
        raise ValueError("Output must be new; refusing to overwrite artifacts")
    files = {}
    for path in sorted(candidate.rglob("*")):
        if path.is_symlink():
            raise ValueError("Symlink in candidate: " + str(path))
        if path.is_file():
            name = path.relative_to(candidate).as_posix()
            allowed = _contract.allowed(name) and name != "addons/sourcemod/extensions/steamworks.ext.2.l4d2.dll"
            if not allowed:
                raise ValueError("Unexpected candidate payload: " + name)
            files[name] = path.read_bytes()
    for required in ("LICENSE", "README.md", "CHANGELOG.md", "plugin-package-map.json",
                     "addons/sourcemod/" + OWNED[0], "addons/sourcemod/" + OWNED[1]):
        if required not in files:
            raise ValueError("Missing candidate payload: " + required)
    if not linux_x86(files["addons/sourcemod/" + OWNED[0]]):
        raise ValueError("Candidate extension is not Linux x86 ELF")
    previous, absent = {}, []
    for name in OWNED:
        path = baseline / name
        if any(p.is_symlink() for p in [path, *path.parents]):
            raise ValueError("Symlink in baseline path: " + name)
        if path.is_file():
            previous[name] = path.read_bytes()
        elif path.exists():
            raise ValueError("Baseline entry is not a file: " + name)
        else:
            absent.append(name)
    binaries = [name for name in (OWNED[0], LEGACY_BINARY) if name in previous]
    if not binaries or any(not linux_x86(previous[name]) for name in binaries):
        raise ValueError("Baseline needs an actual Linux x86 extension")
    if OWNED[1] not in previous:
        raise ValueError("Baseline include missing")
    output.mkdir(parents=True)
    archive(output / f"steamworks-{label}-linux-x86-candidate.zip", files)
    rollback_zip = output / f"steamworks-{label}-rollback.zip"
    archive(rollback_zip, {"addons/sourcemod/" + name: data for name, data in previous.items()})
    # Exercise the archived restore contract in a disposable directory, including
    # removal of a candidate-only gamedata and preservation of unrelated config.
    with tempfile.TemporaryDirectory(prefix="steamworks-rollback-") as temporary:
        sandbox = Path(temporary)
        sentinel = sandbox / "configs/server-private.cfg"
        sentinel.parent.mkdir(parents=True)
        sentinel.write_bytes(b"unrelated configuration sentinel\n")
        for name in OWNED:
            path = sandbox / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(files.get("addons/sourcemod/" + name, b"candidate-only gamedata fixture\n"))
        for name in absent:
            (sandbox / name).unlink()
        with zipfile.ZipFile(rollback_zip) as saved:
            for name in previous:
                (sandbox / name).write_bytes(saved.read("addons/sourcemod/" + name))
        assert all((sandbox / name).read_bytes() == data for name, data in previous.items())
        assert all(not (sandbox / name).exists() for name in absent)
        assert sentinel.read_bytes() == b"unrelated configuration sentinel\n"
    manifest = {"schema_version": 1, "checkpoint": label,
                "generated_utc": datetime.now(timezone.utc).isoformat(),
                "release_approved": False, "deployed": False,
                "platform": "linux-x86", "profile": "l4d2-23990100-linux-x86",
                "candidate_files": {name: sha(data) for name, data in sorted(files.items())},
                "rollback_files": {name: sha(data) for name, data in sorted(previous.items())},
                "rollback_absent_paths": absent,
                "configuration": "No extension-owned config; unrelated server/plugin configs are not changed or bundled.",
                "restore_validation": "passed: actual candidate to archived rollback in disposable filesystem; no server operation",
                "archives": {p.name: sha(p.read_bytes()) for p in sorted(output.glob("*.zip"))}}
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    (output / "SHA256SUMS").write_text("".join(
        sha(p.read_bytes()) + "  " + p.name + "\n"
        for p in sorted(output.iterdir()) if p.is_file()))
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--baseline", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--label", default="p5")
    args = parser.parse_args()
    prepare(args.candidate, args.baseline, args.output, args.label)
    print("REVIEW_ARTIFACTS_OK (local only; release not approved)")


if __name__ == "__main__":
    main()
