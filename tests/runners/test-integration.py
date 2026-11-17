#!/usr/bin/env python3
"""Run local contract suites and reject public API drift. Does not start a server."""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
SUITES = (
    "test-stats-natives", "test-detours", "test-init-coexistence", "test-public-ip",
    "test-steamid-string", "test-typed-boundary", "test-resource-lifecycle",
    "test-restart-forward-semantics",
    "test-binary-evidence", "test-debug-log", "test-http-stream-context",
)


def without_lines(value):
    if isinstance(value, dict):
        return {k: without_lines(v) for k, v in value.items() if k != "line"}
    if isinstance(value, list):
        return [without_lines(v) for v in value]
    return value


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--with-auth-forward", action="store_true",
                        help="Also require standalone SourcePawn VM marshalling tests (SPCOMP, SOURCEPAWN_VM, SOURCEMOD_DIR)")
    parser.add_argument("--with-auth-binding", action="store_true",
                        help="Also require actual SourceHook auth binding tests (MMSOURCE_DIR)")
    parser.add_argument("--with-sourcepawn-environment", action="store_true",
                        help="Also require ASan VM ownership cycles (instrumented SOURCEPAWN_VM, SOURCEMOD_DIR)")
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    spec = importlib.util.spec_from_file_location("inventory", ROOT / "scripts/tools/inventory-steamworks.py")
    inventory = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(inventory)
    current = inventory.api_inventory(ROOT)
    baseline = json.loads((ROOT / "tests/fixtures/api/api-inventory-p0.json").read_text())
    # Compare every contract field; metadata and source hashes are deliberately excluded.
    keys = set(baseline) - {"schema_version", "baseline_commit", "source_sha256"}
    drift = [k for k in sorted(keys) if without_lines(current.get(k)) != without_lines(baseline[k])]
    if drift:
        raise SystemExit("Public API drift: " + ", ".join(drift))
    sdk = os.getenv("STEAMWORKS_SDK_DIR") or str(ROOT / "sdk_155")
    subprocess.run([sys.executable, str(ROOT / "scripts/build/validate-steamworks-sdk.py"), sdk], check=True)
    test_flags = (os.getenv("TEST_CXXFLAGS") or "-m32")
    results = {"public_api": "passed", "test_flags": test_flags,
               "runtime_validation": False, "suites": []}
    failed = False
    commands = [("test-make-interface", [sys.executable, str(ROOT / "tests/python/make_interface_test.py")]),
                ("test-sdk-selection", [sys.executable, str(ROOT / "tests/python/sdk_selection_test.py")]),
                ("test-documentation", [sys.executable, str(ROOT / "tests/python/documentation_test.py")]),
                ("test-review-artifacts", [sys.executable, str(ROOT / "tests/python/review_artifacts_test.py")])]
    commands += [(suite, ["bash", str(ROOT / "tests/runners" / (suite + ".sh"))]) for suite in SUITES]
    if args.with_auth_forward:
        commands.append(("test-auth-forward", ["bash", str(ROOT / "tests/runners/test-auth-forward.sh")]))
    if args.with_auth_binding:
        commands.append(("test-auth-binding", ["bash", str(ROOT / "tests/runners/test-auth-binding.sh")]))
    if args.with_sourcepawn_environment:
        commands.append(("test-sourcepawn-environment", ["bash", str(ROOT / "tests/runners/test-sourcepawn-environment.sh")]))
    for suite, command in commands:
        env = os.environ.copy()
        env["TEST_CXXFLAGS"] = test_flags
        # Keep the established native/decoder toolchain separate from the
        # instrumented VM's matching Clang runtime. No check is disabled.
        if suite in {"test-auth-forward", "test-sourcepawn-environment"}:
            if "VM_TEST_CXX" in env:
                env["CXX"] = env["VM_TEST_CXX"]
            if "VM_TEST_FLAGS" in env:
                env["TEST_CXXFLAGS"] = env["VM_TEST_FLAGS"]
        env["TEST_BUILD_DIR"] = str(output / suite)
        start = time.monotonic()
        with (output / (suite + ".log")).open("w") as log:
            result = subprocess.run(command,
                                    cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT)
        results["suites"].append({"name": suite, "exit_code": result.returncode,
                                  "compiler": env.get("CXX", "g++"),
                                  "test_flags": env.get("TEST_CXXFLAGS", ""),
                                  "duration_seconds": round(time.monotonic() - start, 3)})
        print(suite, result.returncode, flush=True)
        failed |= result.returncode != 0
    (output / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
