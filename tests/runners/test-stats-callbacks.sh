#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUTPUT_DIR="${TEST_BUILD_DIR:-$ROOT_DIR/.build/tests-stats-callbacks}"
SDK_DIR="${STEAMWORKS_SDK_DIR:-$ROOT_DIR/sdk_155}"
python3 "$ROOT_DIR/scripts/build/validate-steamworks-sdk.py" "$SDK_DIR"
mkdir -p "$OUTPUT_DIR"
read -r -a extra_flags <<< "${TEST_CXXFLAGS:-}"
flags=(-std=c++14 -Wall -Wextra -Werror -pedantic -O1 -g
 -fsanitize=address,undefined -fno-sanitize-recover=all "${extra_flags[@]}"
 -I"$ROOT_DIR/tests/support" -I"$ROOT_DIR/extension/sdk" -isystem "$SDK_DIR/public/steam")
"${CXX:-g++}" "${flags[@]}" "$ROOT_DIR/tests/native/stats_callbacks_test.cpp" -o "$OUTPUT_DIR/stats_callbacks_test"
"$OUTPUT_DIR/stats_callbacks_test"

# The former ID-only dispatcher must fail a precise context-isolation check.
python3 - "$ROOT_DIR" "$OUTPUT_DIR/context-blind.h" <<'PY'
from pathlib import Path
import sys
source = Path(sys.argv[1], 'tests/support/steam_callbacks_test_host.h').read_text()
needle = 'CCallbackMgr::GameServer(entry.first) == (context == SteamCallbackContext::GameServer)'
assert source.count(needle) == 1
Path(sys.argv[2]).write_text(source.replace(needle,
    '(context == SteamCallbackContext::Client || context == SteamCallbackContext::GameServer)', 1))
PY
"${CXX:-g++}" "${flags[@]}" "-DSTEAM_CALLBACKS_TEST_HOST=\"$OUTPUT_DIR/context-blind.h\"" \
 "$ROOT_DIR/tests/native/stats_callbacks_test.cpp" -o "$OUTPUT_DIR/context-blind"
if "$OUTPUT_DIR/context-blind" > "$OUTPUT_DIR/context-blind.log" 2>&1; then
 echo 'FAIL: context-blind callback dispatcher passed' >&2; exit 1
fi
grep -q 'FAIL: client unload reaches only client subscribers' "$OUTPUT_DIR/context-blind.log"
if grep -Eq 'AddressSanitizer:|LeakSanitizer:|runtime error:' "$OUTPUT_DIR/context-blind.log"; then
 cat "$OUTPUT_DIR/context-blind.log" >&2; exit 1
fi
echo 'PASS: negative control detects client/GameServer callback context conflation'
