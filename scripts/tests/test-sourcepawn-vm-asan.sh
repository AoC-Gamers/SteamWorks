#!/usr/bin/env bash
# Rebuild and test an isolated VM. No installed library or gameserver is touched.
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
: "${SOURCEMOD_DIR:?Set SOURCEMOD_DIR to the source checkout}"
: "${VM_TEST_BUILD_DIR:?Set VM_TEST_BUILD_DIR to a new output directory}"
: "${SPCOMP:?Set SPCOMP to a Linux SourcePawn compiler}"
if [[ "${VM_TEST_VARIANT:-fixed}" != fixed ]]; then
 echo "This acceptance runner requires the corrected VM variant" >&2; exit 1
fi
export SOURCEPAWN_SOURCE_DIR="${SOURCEPAWN_SOURCE_DIR:-$SOURCEMOD_DIR/sourcepawn}"
export VM_TEST_ARCH="${VM_TEST_ARCH:-x86_64}"
bash "$ROOT_DIR/scripts/tests/helpers/build-test-sourcepawn.sh"
VM_TEST_BUILD_DIR="$(cd "$VM_TEST_BUILD_DIR" && pwd)"
export CXX="${VM_TEST_CXX:-clang++}"
case "$VM_TEST_ARCH" in
 x86) vm_arch_flag=-m32; vm_runtime=libclang_rt.asan-i386.so;;
 x86_64) vm_arch_flag=-m64; vm_runtime=libclang_rt.asan-x86_64.so;;
 *) exit 1;;
esac
vm_runtime_path="$("$CXX" "$vm_arch_flag" --print-file-name="$vm_runtime")"
[[ -f "$vm_runtime_path" ]] || { echo "ASan runtime not found" >&2; exit 1; }
export LD_LIBRARY_PATH="$(dirname "$vm_runtime_path")"
export SOURCEPAWN_VM="$VM_TEST_BUILD_DIR/build/libsourcepawn/linux-$VM_TEST_ARCH/libsourcepawn.so"
# Sized deletion is essential to reproduce the original mismatch with Clang.
# A non-PIE test executable avoids old ASan runtime startup collisions on hosts
# with high mmap entropy. The VM remains PIC; ASLR and checks remain enabled.
export TEST_CXXFLAGS="$vm_arch_flag -fsanitize=address -shared-libasan -fsized-deallocation -fno-pie -no-pie"
export ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:alloc_dealloc_mismatch=1:new_delete_type_mismatch=1
export LSAN_OPTIONS=exitcode=23
export UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
TEST_BUILD_DIR="$VM_TEST_BUILD_DIR/environment-test" bash "$ROOT_DIR/scripts/tests/test-sourcepawn-environment.sh"
TEST_BUILD_DIR="$VM_TEST_BUILD_DIR/auth-test" bash "$ROOT_DIR/scripts/tests/test-auth-forward.sh"
python3 - "$VM_TEST_BUILD_DIR" <<'PY'
from pathlib import Path
import hashlib,json,os,sys
output=Path(sys.argv[1]);vm=Path(os.environ['SOURCEPAWN_VM'])
(output/'acceptance.json').write_text(json.dumps({
 'status':'passed','platform':os.environ['VM_TEST_ARCH'],
 'VM_sha256':hashlib.sha256(vm.read_bytes()).hexdigest(),
 'ASAN_OPTIONS':os.environ['ASAN_OPTIONS'],'LSAN_OPTIONS':os.environ['LSAN_OPTIONS'],
 'UBSAN_OPTIONS':os.environ['UBSAN_OPTIONS'],'test_flags':os.environ['TEST_CXXFLAGS'],
 'VM_instrumented':True,'sized_deallocation':True,'RTTI_enabled':True,
 'environment_cycles':16,'auth_checks':227,'suppression_files':[],
 'installed_runtime_modified':False,'server_deployment':False},indent=2)+'\n')
PY
