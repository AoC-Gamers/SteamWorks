#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUTPUT_DIR="${TEST_BUILD_DIR:-$ROOT_DIR/.build/tests-lifecycle}"
SDK_DIR="${STEAMWORKS_SDK_DIR:-$ROOT_DIR/sdk_155}"
python3 "$ROOT_DIR/scripts/build/validate-steamworks-sdk.py" "$SDK_DIR"
CXX="${CXX:-g++}"
mkdir -p "$OUTPUT_DIR"
read -r -a extra_flags <<< "${TEST_CXXFLAGS:-}"
for suite in client_lifecycle_test http_lifecycle_test forwards_lifecycle_test hooks_lifecycle_test runtime_lifecycle_test lifecycle_profile_rejection_test lifecycle_profile_identity_test; do
 optimize_flags=()
 # Keep the production authentication allocation checked under the optimizer
 # that exposed its x86 padded-object overflow; Check() remains active in NDEBUG.
 if [[ "$suite" == hooks_lifecycle_test ]]; then optimize_flags=(-O3 -DNDEBUG); fi
 "$CXX" -std=c++14 -Wall -Wextra -Werror -pedantic -O1 -g \
  -fsanitize=undefined -fno-sanitize-recover=all "${extra_flags[@]}" "${optimize_flags[@]}" \
  -I"$ROOT_DIR/tests/support/stubs" -I"$ROOT_DIR/tests/support" -I"$ROOT_DIR/extension/sdk" -isystem "$SDK_DIR/public/steam" \
  "$ROOT_DIR/tests/native/$suite.cpp" -o "$OUTPUT_DIR/$suite"
 "$OUTPUT_DIR/$suite"
done

# Exposing provider capacity instead of valid bytes must fail the tail check.
python3 "$ROOT_DIR/tests/helpers/generate-buffer-negatives.py" "$OUTPUT_DIR/negative"
"$CXX" -std=c++14 -Wall -Wextra -Werror -pedantic -O3 -DNDEBUG -g \
 -fsanitize=undefined -fno-sanitize-recover=all "${extra_flags[@]}" \
 -I"$ROOT_DIR/tests/support" -I"$ROOT_DIR/tests/support/stubs" \
 -I"$ROOT_DIR/extension/sdk" -I"$ROOT_DIR/extension/hooks" \
 -isystem "$SDK_DIR/public/steam" "-DGC_HOOK_SOURCE=\"$OUTPUT_DIR/negative/old-gc.cpp\"" \
 "$ROOT_DIR/tests/native/hooks_lifecycle_test.cpp" -o "$OUTPUT_DIR/negative/gc"
if "$OUTPUT_DIR/negative/gc" > "$OUTPUT_DIR/negative/gc.log" 2>&1; then
 echo "FAIL: GC capacity-copy negative control passed" >&2; exit 1
fi
grep -q 'FAIL: GC payload tail contains only initialized zeros' "$OUTPUT_DIR/negative/gc.log"
if grep -Eq 'AddressSanitizer:|LeakSanitizer:|runtime error:' "$OUTPUT_DIR/negative/gc.log"; then
 cat "$OUTPUT_DIR/negative/gc.log" >&2; exit 1
fi
echo "PASS: negative control detects exposure of GC invalid tail"
