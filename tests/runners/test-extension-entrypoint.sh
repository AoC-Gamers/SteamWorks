#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUTPUT_DIR="${TEST_BUILD_DIR:-$ROOT_DIR/.build/tests-extension-entrypoint}"
AMTL_DIR="${AMTL_DIR:-$ROOT_DIR/.deps/exts-linux/sourcemod-1.12/public/amtl}"
mkdir -p "$OUTPUT_DIR"
read -r -a extra_flags <<< "${TEST_CXXFLAGS:-}"
# Compile the production SDK wrapper with its -Wall/-Werror baseline.
# Every runtime assertion and ASan/UBSan check remains enabled.
flags=(-std=c++14 -Wall -Werror -O1 -g
 -fsanitize=address,undefined -fno-sanitize-recover=all "${extra_flags[@]}"
 -isystem "$AMTL_DIR/amtl" -isystem "$AMTL_DIR")
"${CXX:-g++}" "${flags[@]}" "$ROOT_DIR/tests/native/extension_entrypoint_test.cpp" -o "$OUTPUT_DIR/entrypoint"
"$OUTPUT_DIR/entrypoint"

# Restore only the former factory behavior. The same regression must reject it
# before any sanitizer fault; do not turn a crash into a passing negative.
python3 - "$ROOT_DIR" "$OUTPUT_DIR/old-factory.cpp" <<'PYNEG'
from pathlib import Path
import sys
source = Path(sys.argv[1], 'extension/sdk/smsdk_ext.cpp').read_text()
needle = 'if (extensionLoadState != ExtensionLoadState::Available) { return NULL; }'
assert source.count(needle) == 1
Path(sys.argv[2]).write_text(source.replace(needle, '', 1))
PYNEG
"${CXX:-g++}" "${flags[@]}" -I"$ROOT_DIR/extension/sdk" \
 "-DSDK_ENTRY_SOURCE=\"$OUTPUT_DIR/old-factory.cpp\"" \
 "$ROOT_DIR/tests/native/extension_entrypoint_test.cpp" -o "$OUTPUT_DIR/old-factory"
if "$OUTPUT_DIR/old-factory" > "$OUTPUT_DIR/old-factory.log" 2>&1; then
 echo 'FAIL: former factory behavior passed duplicate-load regression' >&2; exit 1
fi
grep -q 'FAIL: factory blocks reentry during initialization' "$OUTPUT_DIR/old-factory.log"
if grep -Eq 'AddressSanitizer:|LeakSanitizer:|runtime error:' "$OUTPUT_DIR/old-factory.log"; then
 cat "$OUTPUT_DIR/old-factory.log" >&2; exit 1
fi
echo 'PASS: negative control detects the former unguarded factory'
