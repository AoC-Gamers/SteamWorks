#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT_DIR="${TEST_BUILD_DIR:-$ROOT_DIR/.build/tests-auth-forward}"
SM_DIR="${SOURCEMOD_DIR:-$ROOT_DIR/../sourcemod}"
SDK_DIR="${STEAMWORKS_SDK_DIR:-$ROOT_DIR/sdk_158a}"
: "${SPCOMP:?Set SPCOMP to the Linux SourcePawn compiler}"
: "${SOURCEPAWN_VM:?Set SOURCEPAWN_VM to a same-architecture SourcePawn VM library}"
mkdir -p "$OUTPUT_DIR"
# Preserve the implementation verbatim; omit only the unrelated global manager
# and dependency includes. No modified SourceMod file is written to its checkout.
python3 - "$SM_DIR" "$OUTPUT_DIR" "$SPCOMP" "$SOURCEPAWN_VM" <<'PY'
from pathlib import Path
import hashlib,json,sys
source,output,compiler,vm=map(Path,sys.argv[1:])
header=source/'core/logic/ForwardSys.h'
implementation=source/'core/logic/ForwardSys.cpp'
h,c=header.read_text(),implementation.read_text()
declaration=h[h.index('class CForward :'):h.index('class CForwardManager :')]
methods=c[c.index('CForward::CForward('):]
(output/'sourcemod-forward.inc').write_text(declaration+methods)
(output/'source-inputs.json').write_text(json.dumps({
 str(p.relative_to(source)):hashlib.sha256(p.read_bytes()).hexdigest()
 for p in [header,implementation,source/'public/IForwardSys.h',source/'public/ReentrantList.h',source/'sourcepawn/include/sp_vm_api.h']},indent=2)+'\n')
(output/'runtime-inputs.json').write_text(json.dumps({
 'compiler_sha256':hashlib.sha256(compiler.read_bytes()).hexdigest(),
 'VM_sha256':hashlib.sha256(vm.read_bytes()).hexdigest(),
 'VM_filename':vm.name},indent=2)+'\n')
PY
"$SPCOMP" "$ROOT_DIR/tests/auth_forward_receiver.sp" -o"$OUTPUT_DIR/auth_forward_receiver.smx"
read -r -a extra_flags <<< "${TEST_CXXFLAGS:-}"
"${CXX:-g++}" -std=c++14 -Wall -Wextra -Werror -O1 -g \
 -fsanitize=undefined -fno-sanitize-recover=all "${extra_flags[@]}" \
 -I"$OUTPUT_DIR" -I"$ROOT_DIR/tests/stubs" -I"$ROOT_DIR/extension/sdk" \
 -isystem "$SDK_DIR/public/steam" -isystem "$SM_DIR/public" \
 -isystem "$SM_DIR/public/amtl" -isystem "$SM_DIR/sourcepawn/include" \
 "$ROOT_DIR/tests/auth_forward_test.cpp" -ldl -o "$OUTPUT_DIR/auth_forward_test"
"$OUTPUT_DIR/auth_forward_test" "$SOURCEPAWN_VM" "$OUTPUT_DIR/auth_forward_receiver.smx"
