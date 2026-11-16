#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT_DIR="${TEST_BUILD_DIR:-$ROOT_DIR/.build/tests-init-coexistence}"
mkdir -p "$OUTPUT_DIR"
read -r -a flags <<< "${TEST_CXXFLAGS:--m32}"
"${CXX:-g++}" -std=c++14 -Wall -Wextra -Werror -pedantic -O1 -g \
 -fsanitize=undefined -fno-sanitize-recover=all "${flags[@]}" \
 -I"$ROOT_DIR/extension/sdk" -isystem "${STEAMWORKS_SDK_DIR:-$ROOT_DIR/sdk_158a}/public/steam" \
 "$ROOT_DIR/tests/init_coexistence_test.cpp" -o "$OUTPUT_DIR/init_coexistence_test"
"$OUTPUT_DIR/init_coexistence_test"
"${CXX:-g++}" -std=c++14 -Wall -Wextra -Werror -pedantic -O1 -g \
 -fsanitize=undefined -fno-sanitize-recover=all "${flags[@]}" \
 "$ROOT_DIR/tests/init_call_profile_test.cpp" -o "$OUTPUT_DIR/init_call_profile_test"
"$OUTPUT_DIR/init_call_profile_test"

# Negative control: preserve the SDK call without the observer, reproducing the
# lost successful-init event while leaving the consumer and original executable.
python3 - "$ROOT_DIR" "$OUTPUT_DIR" <<'PY'
from pathlib import Path
import sys
root,out=map(Path,sys.argv[1:]);s=(root/'extension/core/swinitcallsite.h').read_text()
old='static_cast<uint32_t>(reinterpret_cast<uintptr_t>(callback))'
assert s.count(old)==1
(out/'missing-observer.h').write_text(s.replace(old,'static_cast<uint32_t>(reinterpret_cast<uintptr_t>(target))'))
PY
"${CXX:-g++}" -std=c++14 -Wall -Wextra -Werror -pedantic -O1 -g \
 -fsanitize=undefined -fno-sanitize-recover=all "${flags[@]}" \
 -I"$ROOT_DIR/extension/sdk" -isystem "${STEAMWORKS_SDK_DIR:-$ROOT_DIR/sdk_158a}/public/steam" \
 -include "$OUTPUT_DIR/missing-observer.h" "$ROOT_DIR/tests/init_coexistence_test.cpp" -o "$OUTPUT_DIR/missing_observer_test"
if "$OUTPUT_DIR/missing_observer_test" > "$OUTPUT_DIR/missing-observer.log" 2>&1; then
 echo "FAIL: missing observer escaped detection" >&2;exit 1
fi
grep -q 'FAIL: only original success reactivates production runtime' "$OUTPUT_DIR/missing-observer.log"
if grep -Eq 'AddressSanitizer:|LeakSanitizer:|runtime error:' "$OUTPUT_DIR/missing-observer.log"; then
 cat "$OUTPUT_DIR/missing-observer.log" >&2;exit 1
fi
echo "PASS: negative control detects init event lost behind consumer"
