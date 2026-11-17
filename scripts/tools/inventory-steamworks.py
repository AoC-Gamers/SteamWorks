#!/usr/bin/env python3
"""Extract the current SteamWorks source contract; never load Steam or game binaries.

This is a source inventory, not a SourcePawn/C++ parser or runtime acceptance test.
Consumer discovery uses rg and excludes dependency/build trees. References do not
prove that a plugin is installed, reachable, or compiled against this checkout.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path


LEXEMES = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*.*?\*/|//[^\n]*', re.S)
EXCLUDED_DIRS = (
    ".git", "node_modules", "deps", ".deps", ".build", "vendor", "vendors",
    "sdk", "sdk_*", "steamworks-sdk", ".venv", "build", "dist", "tmp", ".tmp",
    "third_party", "thirdparty",
)


def mask(text: str, *, strings: bool = False) -> str:
    def replace(match: re.Match[str]) -> str:
        if not strings and match[0][0] in "\"'":
            return match[0]
        if strings and match[0][0] in "\"'":
            return "0" + re.sub(r"[^\n]", " ", match[0][1:])
        return re.sub(r"[^\n]", " ", match[0])
    return LEXEMES.sub(replace, text)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def line_at(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def revision(root: Path) -> str | None:
    # Source-review archives have no Git metadata. A surrounding checkout is
    # not the archive's revision either; do not attribute its HEAD to this tree.
    top = subprocess.run(["git", "-C", str(root), "rev-parse", "--show-toplevel"],
                         capture_output=True, text=True)
    if top.returncode != 0 or Path(top.stdout.strip()).resolve() != root.resolve():
        return None
    head = subprocess.run(["git", "-C", str(root), "rev-parse", "HEAD"],
                          capture_output=True, text=True)
    return head.stdout.strip() if head.returncode == 0 else None


def argument_count(source: str, start: int) -> int:
    """Count a masked call's arguments, respecting nested calls and arrays."""
    stack = []
    commas = 0
    content = False
    for char in source[start:]:
        if char == ")" and not stack:
            return commas + 1 if content else 0
        if char in "([{":
            stack.append(char)
        elif char in ")]}":
            if not stack:
                raise ValueError("Unbalanced SourcePawn argument list")
            stack.pop()
        elif char == "," and not stack:
            commas += 1
        if not char.isspace():
            content = True
    raise ValueError("Unterminated SourcePawn argument list")


def api_inventory(root: Path) -> dict:
    include = root / "addons/sourcemod/scripting/include/steamworks.inc"
    text = mask(include.read_text())
    declarations = []
    for m in re.finditer(r"\b(native|forward)\s+(\w+)\s+(\w+)\s*\((.*?)\)\s*;", text, re.S):
        declarations.append({
            "kind": m[1], "name": m[3], "return_type": m[2],
            "parameters": [" ".join(p.split()) for p in m[4].split(",") if p.strip()],
            "signature": " ".join(m[0].split()), "line": line_at(text, m.start()),
        })
    enums = []
    for m in re.finditer(r"\benum\s+(\w+)\s*\{(.*?)\}\s*;", text, re.S):
        members, next_value = [], 0
        for entry in m[2].split(","):
            entry = entry.strip()
            if not entry:
                continue
            name, separator, literal = entry.partition("=")
            value = int(literal.strip(), 0) if separator else next_value
            members.append({"name": name.strip(), "value": value})
            next_value = value + 1
        enums.append({"name": m[1], "members": members, "line": line_at(text, m.start())})
    callbacks = []
    for m in re.finditer(r"\btypeset\s+(\w+)\s*\{(.*?)\}\s*;?", text, re.S):
        callbacks.append({"name": m[1], "line": line_at(text, m.start()),
                          "signatures": [" ".join(s.split()) for s in re.findall(r"function\s+[^;]+;", m[2])]})
    optional = sorted(set(re.findall(r'MarkNativeAsOptional\("([^"]+)"\)', text)))
    registrations, forwards, hashes = [], [], {}
    for path in sorted((root / "extension").rglob("*")):
        if path.suffix not in {".cpp", ".h", ".c"}:
            continue
        relative = path.relative_to(root).as_posix()
        hashes[relative] = digest(path)
        source = mask(path.read_text())
        for m in re.finditer(r'\{\s*"((?:SteamWorks_|SW_)\w+)"\s*,\s*(\w+)\s*\}', source):
            registrations.append({"name": m[1], "implementation": m[2], "file": relative,
                                  "line": line_at(source, m.start())})
        for m in re.finditer(r'CreateForward\(\s*"([^"]+)"\s*,\s*(.*?)\);', source, re.S):
            forwards.append({"name": m[1], "registration_arguments": " ".join(m[2].split()),
                             "file": relative, "line": line_at(source, m.start())})
    native_names = {d["name"] for d in declarations if d["kind"] == "native"}
    forward_names = {d["name"] for d in declarations if d["kind"] == "forward"}
    registered_names = {r["name"] for r in registrations}
    registered_forwards = {r["name"] for r in forwards}
    binding = re.search(r"public\s+Extension\s+(\w+)\s*=\s*\{(.*?)\};", text, re.S)
    if binding is None:
        raise ValueError("SourcePawn extension binding not found")
    binding_properties = dict(re.findall(r'(name|file)\s*=\s*"([^"]+)"', binding[2]))
    library = re.search(r'RegisterLibrary\(myself,\s*"([^"]+)"\)',
                        mask((root / "extension/extension.cpp").read_text()))
    if library is None:
        raise ValueError("SourceMod library registration not found")
    hashes["addons/sourcemod/scripting/include/steamworks.inc"] = digest(include)
    for relative in ("tests/runtime/steamwork_test.sp", "extension/AMBuilder", "AMBuildScript",
                     "configure.py", "Makefile", "PackageScript", "scripts/package/package-map.json"):
        hashes[relative] = digest(root / relative)
    return {
        "schema_version": 1, "baseline_commit": revision(root), "source_sha256": hashes,
        "extension_binding": {"symbol": binding[1], "library_registered": library[1],
                              **binding_properties,
                              "conditional_source": " ".join(binding[2].split())},
        "declarations": declarations, "enums": enums, "callback_typesets": callbacks,
        "native_registrations": registrations, "forward_registrations": forwards,
        "optional_natives": optional,
        "comparison": {
            "declared_natives_not_registered": sorted(native_names - registered_names),
            "registered_natives_not_declared": sorted(registered_names - native_names),
            "declared_forwards_not_registered": sorted(forward_names - registered_forwards),
            "registered_forwards_not_declared": sorted(registered_forwards - forward_names),
            "declared_natives_not_optional": sorted(native_names - set(optional)),
            "optional_natives_not_declared": sorted(set(optional) - native_names),
        },
    }


def consumers_inventory(scan_root: Path, api: dict) -> dict:
    cmd = ["rg", "--files", "--hidden", "-g", "*.sp", "-g", "*.inc"]
    for directory in EXCLUDED_DIRS:
        cmd += ["-g", f"!**/{directory}/**"]
    cmd.append(str(scan_root))
    paths = subprocess.check_output(cmd, text=True).splitlines()
    files = []
    repositories = {}
    declarations = {d["name"]: d for d in api["declarations"]}
    arity_mismatches = []
    for name in sorted(paths):
        path = Path(name)
        source = mask(path.read_text(errors="replace"), strings=True)
        hits = []
        for m in re.finditer(r"\b(SteamWorks_\w+|SW_OnValidateClient)\s*\(", source):
            prefix = source[source.rfind("\n", 0, m.start()) + 1:m.start()].strip()
            role = ("declaration" if re.match(r"(?:native|forward)\b", prefix) else
                    "handler" if re.match(r"public\b", prefix) else "call_or_reference")
            arity = argument_count(source, m.end())
            hit = {"symbol": m[1], "line": line_at(source, m.start()), "role": role,
                   "argument_count": arity}
            hits.append(hit)
            if role == "call_or_reference" and m[1] in declarations:
                parameters = declarations[m[1]]["parameters"]
                required = sum("=" not in p for p in parameters)
                if not required <= arity <= len(parameters):
                    arity_mismatches.append({"file": path.relative_to(scan_root).as_posix(),
                                             **hit, "current_required": required,
                                             "current_maximum": len(parameters)})
        if hits:
            record = {"file": path.relative_to(scan_root).as_posix(),
                      "sha256": digest(path), "hits": hits}
            if all(h["role"] == "declaration" for h in hits):
                signatures = {m[3]: " ".join(m[0].split()) for m in re.finditer(
                    r"\b(native|forward)\s+(\w+)\s+(\w+)\s*\((.*?)\)\s*;",
                    mask(path.read_text(errors="replace")), re.S)}
                record["include_comparison"] = {
                    "missing_names": sorted(set(declarations) - set(signatures)),
                    "extra_names": sorted(set(signatures) - set(declarations)),
                    "different_signature_text": [
                        {"name": name, "consumer_signature": signatures[name],
                         "current_signature": declarations[name]["signature"]}
                        for name in sorted(set(signatures) & set(declarations))
                        if signatures[name] != declarations[name]["signature"]
                    ],
                }
            try:
                repository = Path(subprocess.check_output(
                    ["git", "-C", str(path.parent), "rev-parse", "--show-toplevel"],
                    text=True, stderr=subprocess.DEVNULL).strip())
                label = repository.relative_to(scan_root).as_posix()
                def git(*arguments: str) -> str:
                    return subprocess.check_output(["git", "-C", str(repository), *arguments], text=True).strip()
                if label not in repositories:
                    repositories[label] = {
                        "commit": revision(repository), "branch": git("branch", "--show-current"),
                        "tracked_changes_present": bool(git("status", "--porcelain", "--untracked-files=no")),
                    }
                record["repository"] = label
                record["file_tracked_modified"] = bool(git("diff", "HEAD", "--name-only", "--", str(path)))
            except (subprocess.CalledProcessError, ValueError):
                record["repository"] = None
            files.append(record)
    known = {d["name"] for d in api["declarations"]}
    used = {h["symbol"] for f in files for h in f["hits"] if h["role"] != "declaration"}
    return {
        "schema_version": 1, "scan_root_label": scan_root.name,
        "source_files_scanned": len(paths), "excluded_directories": list(EXCLUDED_DIRS),
        "limits": "Lexical .sp/.inc references only; excludes ignored files and does not establish deployment or reachability.",
        "files": files, "repositories": repositories,
        "consumer_symbols_not_in_public_include": sorted(used - known),
        "calls_outside_current_declared_arity": arity_mismatches,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--consumer-root", type=Path)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    api = api_inventory(args.root.resolve())
    args.output_dir.mkdir(parents=True, exist_ok=True)
    (args.output_dir / "api-inventory.json").write_text(json.dumps(api, indent=2) + "\n")
    if args.consumer_root:
        consumers = consumers_inventory(args.consumer_root.resolve(), api)
        (args.output_dir / "consumers-inventory.json").write_text(json.dumps(consumers, indent=2) + "\n")
        print(f"Consumers: {len(consumers['files'])} files / {consumers['source_files_scanned']} scanned")
    print(json.dumps(api["comparison"], indent=2))


if __name__ == "__main__":
    main()
