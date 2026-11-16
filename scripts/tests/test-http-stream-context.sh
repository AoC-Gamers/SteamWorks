#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUTPUT_DIR="${TEST_BUILD_DIR:-$ROOT_DIR/.build/tests-http-stream-context}"
SDK_DIR="${STEAMWORKS_SDK_DIR:-$ROOT_DIR/sdk_158a}"
CXX="${CXX:-g++}"
mkdir -p "$OUTPUT_DIR"
read -r -a extra_flags <<< "${TEST_CXXFLAGS:--m32}"
flags=(-std=c++14 -Wall -Wextra -Werror -pedantic -O1 -g
 -fsanitize=undefined -fno-sanitize-recover=all "${extra_flags[@]}"
 -I"$ROOT_DIR/tests/support/stubs" -I"$ROOT_DIR/tests/support" -I"$ROOT_DIR/extension/sdk" -I"$ROOT_DIR/extension/http"
 -isystem "$SDK_DIR/public/steam")
"$CXX" "${flags[@]}" "$ROOT_DIR/tests/native/http_stream_context_test.cpp" -o "$OUTPUT_DIR/http_stream_context_test"
"$OUTPUT_DIR/http_stream_context_test"
# Prove the independent wire fixture detects the original context loss.
python3 - "$ROOT_DIR" "$OUTPUT_DIR" <<'PY'
from pathlib import Path
import sys
root,out=map(Path,sys.argv[1:])
source=(root/'extension/http/swhttprequest.cpp').read_text()
for forward in ('pHeadersReceivedForward','pDataReceivedForward'):
    old=f'PushContextValue({forward}, m_contextValue);'
    assert source.count(old)==1
    source=source.replace(old,f'PushContextValue({forward}, pRequest->m_ulContextValue);')
(out/'old-context.cpp').write_text(source)
PY
"$CXX" "${flags[@]}" "-DHTTP_REQUEST_SOURCE=\"$OUTPUT_DIR/old-context.cpp\"" \
 "$ROOT_DIR/tests/native/http_stream_context_test.cpp" -o "$OUTPUT_DIR/old_context_test"
if "$OUTPUT_DIR/old_context_test" > "$OUTPUT_DIR/old-context.log" 2>&1; then
 echo "FAIL: original streaming context loss was not detected" >&2; exit 1
fi
grep -q 'FAIL: headers preserve both accepted context cells' "$OUTPUT_DIR/old-context.log"
if grep -Eq 'AddressSanitizer:|LeakSanitizer:|runtime error:' "$OUTPUT_DIR/old-context.log"; then
 cat "$OUTPUT_DIR/old-context.log" >&2; exit 1
fi
echo "PASS: negative control detects original streaming context loss"
