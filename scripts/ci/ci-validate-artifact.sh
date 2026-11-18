#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARTIFACT_DIR="${EXTS_ARTIFACT_DIR:-$ROOT_DIR/dist/sourcemod/artifact}"
PACKAGE_MAP_PATH="$ROOT_DIR/scripts/package/package-map.json"
PLATFORM="${EXTS_PLATFORM:-linux}"

if [[ ! -d "$ARTIFACT_DIR" ]]; then
  echo "Extension artifact directory not found at $ARTIFACT_DIR" >&2
  exit 1
fi

PYTHON_BIN="$(command -v python3 >/dev/null 2>&1 && echo python3 || echo python)"

"$PYTHON_BIN" - "$ROOT_DIR" "$ARTIFACT_DIR" "$PACKAGE_MAP_PATH" "$PLATFORM" <<'PY'
import importlib.util
from pathlib import Path
import json
import os
import sys


def validate_manifest_tree(source_root: str, artifact_root: str, manifest: dict) -> None:
    if manifest.get("all", False):
        if not os.path.isdir(source_root):
            raise SystemExit(f"Missing source directory for validation: {source_root}")
        if not os.path.isdir(artifact_root):
            raise SystemExit(f"Missing artifact directory: {artifact_root}")
        source_entries = sorted(os.listdir(source_root))
        artifact_entries = sorted(os.listdir(artifact_root))
        if source_entries != artifact_entries:
            raise SystemExit(f"Directory mismatch for {artifact_root}: expected {source_entries}, got {artifact_entries}")

    for relative_file in manifest.get("files", []):
        source_path = os.path.join(source_root, relative_file)
        artifact_path = os.path.join(artifact_root, relative_file)
        if not os.path.isfile(source_path):
            raise SystemExit(f"Missing source artifact file declared in manifest: {source_path}")
        if not os.path.isfile(artifact_path):
            raise SystemExit(f"Missing packaged artifact file: {artifact_path}")

    for relative_dir in manifest.get("dirs", []):
        source_path = os.path.join(source_root, relative_dir)
        artifact_path = os.path.join(artifact_root, relative_dir)
        if not os.path.isdir(source_path):
            raise SystemExit(f"Missing source artifact directory declared in manifest: {source_path}")
        if not os.path.isdir(artifact_path):
            raise SystemExit(f"Missing packaged artifact directory: {artifact_path}")

    for key, value in manifest.items():
        if key in {"all", "files", "dirs"}:
            continue
        if isinstance(value, dict):
            validate_manifest_tree(os.path.join(source_root, key), os.path.join(artifact_root, key), value)


root_dir, artifact_dir, package_map_path, platform = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4]

spec = importlib.util.spec_from_file_location("artifact_contract", Path(root_dir) / "scripts/package/artifact_contract.py")
contract = importlib.util.module_from_spec(spec)
spec.loader.exec_module(contract)
contract.validate_tree(Path(artifact_dir))
for runtime_name in contract.RUNTIME:
    if runtime_name.endswith(".dll" if platform == "linux" else ".so") and (Path(artifact_dir) / runtime_name).exists():
        raise SystemExit("Artifact contains extension for the wrong platform")

with open(package_map_path, "r", encoding="utf-8") as fh:
    package_map = json.load(fh)

extensions_dir = os.path.join(artifact_dir, "addons", "sourcemod", "extensions")
expected_suffix = ".dll" if platform == "windows" else ".so"

for bucket, extensions in package_map.get("build", {}).get("extensions", {}).items():
    for extension in extensions:
        filename = f"{extension}.ext.2.l4d2{expected_suffix}"
        extension_path = (
            os.path.join(extensions_dir, filename)
            if bucket == "root"
            else os.path.join(extensions_dir, bucket, filename)
        )
        if not os.path.isfile(extension_path):
            raise SystemExit(f"Missing compiled extension: {extension_path}")
        if platform == "linux":
            with open(extension_path, "rb") as extension_file:
                header = extension_file.read(52)
            # The L4D2 runtime requires little-endian ELF32, ET_DYN, EM_386.
            if (len(header) < 52 or header[:7] != b"\x7fELF\x01\x01\x01"
                    or int.from_bytes(header[16:18], "little") != 3
                    or int.from_bytes(header[18:20], "little") != 3):
                raise SystemExit(f"L4D2 requires a Linux x86 ELF32 extension: {extension_path}")

source_root = os.path.join(root_dir, "addons", "sourcemod", "scripting")
artifact_root = os.path.join(artifact_dir, "addons", "sourcemod", "scripting")
artifact_manifest = package_map.get("artifact", {}).get("addons", {}).get("sourcemod", {}).get("scripting", {})
validate_manifest_tree(source_root, artifact_root, artifact_manifest)

print("ARTIFACT_VALIDATION_OK")
PY
