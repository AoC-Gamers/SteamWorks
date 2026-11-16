#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT_DIR="${TEST_BUILD_DIR:-$ROOT_DIR/.build/tests-binary-evidence}"
SDK_DIR="${STEAMWORKS_SDK_DIR:-$ROOT_DIR/sdk_158a}"
mkdir -p "$OUTPUT_DIR"
python3 "$ROOT_DIR/tests/runtime_evidence_test.py"
python3 "$ROOT_DIR/scripts/validate-runtime-evidence.py" "$ROOT_DIR/docs/runtime-compatibility/p4/linux-x86-candidate.json"
read -r -a extra_flags <<< "${TEST_CXXFLAGS:-}"
if [[ " ${TEST_CXXFLAGS:-} " == *" -m32 "* ]]; then
 # Production x86 decoder uses unaligned integer accesses. This checks its real
 # machine-code relocation behavior with normal x86 compilation, not sanitizer acceptance.
 "${CC:-gcc}" -m32 -O1 -I"$ROOT_DIR/extension" -c "$ROOT_DIR/extension/asm/asm.c" -o "$OUTPUT_DIR/asm.o"
 "${CXX:-g++}" -std=c++14 -Wall -Wextra -Werror -O1 "${extra_flags[@]}" \
  -I"$ROOT_DIR/extension" "$ROOT_DIR/tests/binary_trampoline_test.cpp" "$OUTPUT_DIR/asm.o" -ldl -o "$OUTPUT_DIR/binary_trampoline_test"
 "$OUTPUT_DIR/binary_trampoline_test" "$SDK_DIR/redistributable_bin/linux32/libsteam_api.so"
 if [[ -n "${RUNTIME_MODULE_PATH:-}" ]]; then
  "$OUTPUT_DIR/binary_trampoline_test" "$RUNTIME_MODULE_PATH" runtime
 fi
else
 echo "SKIP: x86 PIC-prefix execution requires TEST_CXXFLAGS=-m32 and a multilib toolchain"
fi
