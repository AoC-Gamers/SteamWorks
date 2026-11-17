#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUTPUT_DIR="${TEST_BUILD_DIR:-$ROOT_DIR/.build/tests-ticket-ownership}"
SDK_DIR="${STEAMWORKS_SDK_DIR:-$ROOT_DIR/sdk_155}"
python3 "$ROOT_DIR/scripts/build/validate-steamworks-sdk.py" "$SDK_DIR"
mkdir -p "$OUTPUT_DIR"
read -r -a extra_flags <<< "${TEST_CXXFLAGS:--m32}"
flags=(-std=c++14 -Wall -Wextra -Werror -pedantic -O1 -g
 -fsanitize=address,undefined -fno-sanitize-recover=all "${extra_flags[@]}"
 -I"$ROOT_DIR/extension/steamtools" -isystem "$SDK_DIR/public/steam")
"${CXX:-g++}" "${flags[@]}" "$ROOT_DIR/tests/native/ticket_ownership_test.cpp" -o "$OUTPUT_DIR/ticket_test"
"$OUTPUT_DIR/ticket_test"

python3 - "$ROOT_DIR" "$OUTPUT_DIR" <<'PY'
from pathlib import Path
import sys
root,out=map(Path,sys.argv[1:])
source=(root/'extension/steamtools/ticket.h').read_text()
needle='DLCInfo() : appid(0), subcount(0), pSub(nullptr)'
assert source.count(needle)==1
(out/'old-init.h').write_text(source.replace(needle,'DLCInfo() : pSub(nullptr)',1))
for name in ('OwnershipSection','AuthBlob'):
    old=source.replace(f'\t{name}(const {name}&) = delete;\n','').replace(f'\t{name}& operator=(const {name}&) = delete;\n','')
    assert old!=source
    (out/f'copyable-{name}.h').write_text(old)
PY
"${CXX:-g++}" "${flags[@]}" "-DTICKET_SOURCE=\"$OUTPUT_DIR/old-init.h\"" \
 "$ROOT_DIR/tests/native/ticket_ownership_test.cpp" -o "$OUTPUT_DIR/old-init"
if "$OUTPUT_DIR/old-init" > "$OUTPUT_DIR/old-init.log" 2>&1; then
 echo 'FAIL: old uninitialized DLC passed' >&2; exit 1
fi
grep -q 'FAIL: default DLC appid is zero' "$OUTPUT_DIR/old-init.log"
if grep -Eq 'AddressSanitizer:|LeakSanitizer:|runtime error:' "$OUTPUT_DIR/old-init.log"; then
 cat "$OUTPUT_DIR/old-init.log" >&2; exit 1
fi
for name in OwnershipSection AuthBlob; do
 if "${CXX:-g++}" "${flags[@]}" "-DTICKET_SOURCE=\"$OUTPUT_DIR/copyable-$name.h\"" \
  "$ROOT_DIR/tests/native/ticket_ownership_test.cpp" -o "$OUTPUT_DIR/copyable-$name" > "$OUTPUT_DIR/copyable-$name.log" 2>&1; then
  echo "FAIL: shallow-copy $name compiled" >&2; exit 1
 fi
 grep -q "$name must reject copying" "$OUTPUT_DIR/copyable-$name.log"
done
echo 'PASS: old initialization and shallow-copy negatives rejected without sanitizer faults'
