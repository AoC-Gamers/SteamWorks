#!/usr/bin/env bash
# Build a private test VM from pinned sources; never replace installed runtimes.
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
: "${SOURCEPAWN_SOURCE_DIR:?Set SOURCEPAWN_SOURCE_DIR to the pinned SourcePawn checkout}"
: "${VM_TEST_BUILD_DIR:?Set VM_TEST_BUILD_DIR to a new private build directory}"
: "${AMBUILD_PYTHON:?Set AMBUILD_PYTHON to the Python interpreter with AMBuild}"
: "${AMBUILD_BIN:?Set AMBUILD_BIN to the AMBuild command}"
VM_TEST_ARCH="${VM_TEST_ARCH:-x86_64}"
VM_TEST_VARIANT="${VM_TEST_VARIANT:-fixed}"
case "$VM_TEST_ARCH" in x86|x86_64) ;; *) echo "Unsupported test architecture" >&2; exit 1;; esac
case "$VM_TEST_VARIANT" in fixed|baseline) ;; *) echo "Unsupported test variant" >&2; exit 1;; esac
python3 - "$SOURCEPAWN_SOURCE_DIR" "$VM_TEST_BUILD_DIR" "$ROOT_DIR/patches/sourcepawn" "$VM_TEST_VARIANT" <<'PY'
from pathlib import Path
import hashlib,json,shutil,subprocess,sys
source,output,patches=map(lambda p:Path(p).resolve(),sys.argv[1:4])
variant=sys.argv[4]
expected='11b22edb634b9764d19fd28699e03289cfd18520'
git=['git','-c','safe.directory='+str(source),'-C',str(source)]
revision=subprocess.check_output(git+['rev-parse','HEAD'],text=True).strip()
if revision!=expected:
 raise SystemExit('Unreviewed SourcePawn revision: '+revision)
if subprocess.check_output(git+['diff','HEAD','--']):
 raise SystemExit('SourcePawn has tracked changes; refusing an ambiguous baseline')
if output.exists():
 raise SystemExit('Output must be new: '+str(output))
output.mkdir(parents=True)
shutil.copytree(source,output/'source',ignore=shutil.ignore_patterns('.git','__pycache__','*.pyc'))
hashes={p.relative_to(output/'source').as_posix():hashlib.sha256(p.read_bytes()).hexdigest()
        for p in sorted((output/'source').rglob('*')) if p.is_file()}
patch_hashes={}
names=['test-vm-only.patch']
if variant=='fixed':names.insert(0,'environment-ownership.patch')
for name in names:
 patch=patches/name
 subprocess.run(['patch','--batch','--forward','-p1','-i',str(patch)],cwd=output/'source',check=True)
 patch_hashes[name]=hashlib.sha256(patch.read_bytes()).hexdigest()
after={p.relative_to(output/'source').as_posix():hashlib.sha256(p.read_bytes()).hexdigest()
       for p in sorted((output/'source').rglob('*')) if p.is_file()}
changed=sorted(p for p in hashes if hashes[p]!=after[p])
expected_changed=['AMBuildScript','vm/environment.cpp','vm/environment.h'] if variant=='fixed' else ['AMBuildScript']
if changed!=expected_changed or hashes.keys()!=after.keys():
 raise SystemExit('Patch unexpectedly changed files: '+str(changed))
if any(hashes[p]!=after[p] for p in hashes if p.startswith('include/')):
 raise SystemExit('Public SourcePawn API changed')
(output/'provenance.json').write_text(json.dumps({'revision':revision,'variant':variant,
 'patches_sha256':patch_hashes,
 'before_sha256':hashes,'after_sha256':after,'changed_files':changed,
 'public_headers_unchanged':True,'installed_runtime_modified':False},indent=2)+'\n')
(output/'build').mkdir()
PY
VM_TEST_BUILD_DIR="$(cd "$VM_TEST_BUILD_DIR" && pwd)"
cd "$VM_TEST_BUILD_DIR/build"
# SourcePawn's supported Clang ASan mode instruments compilation AND linking.
# Do not disable sized deletion, mismatch detection, leaks or any sanitizer check.
export CC="${VM_TEST_CC:-clang}"
export CXX="${VM_TEST_CXX:-clang++} -fsized-deallocation -fno-omit-frame-pointer"
"$AMBUILD_PYTHON" ../source/configure.py --enable-debug --enable-optimize --enable-asan --targets "$VM_TEST_ARCH"
"$AMBUILD_BIN" -j "${VM_TEST_JOBS:-2}"
