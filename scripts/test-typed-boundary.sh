#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT_DIR="${TEST_BUILD_DIR:-$ROOT_DIR/.build/tests-typed}"
SDK_DIR="${STEAMWORKS_SDK_DIR:-$ROOT_DIR/sdk_158a}"
CXX="${CXX:-g++}"
mkdir -p "$OUTPUT_DIR"
read -r -a extra_flags <<< "${TEST_CXXFLAGS:-}"
for suite in interface_registry_test typed_boundary_test http_gc_boundary_test; do
 "$CXX" -std=c++14 -Wall -Wextra -Werror -pedantic -O1 -g \
  -fsanitize=undefined -fno-sanitize-recover=all "${extra_flags[@]}" \
  -I"$ROOT_DIR/extension/sdk" -isystem "$SDK_DIR/public/steam" \
  "$ROOT_DIR/tests/$suite.cpp" -o "$OUTPUT_DIR/$suite"
 "$OUTPUT_DIR/$suite"
done

# Windows headers define max(a,b); the real boundary must compile unchanged
# with that macro still active, including the authentication allocation bound.
"$CXX" -std=c++14 -Wall -Wextra -Werror -pedantic -O1 -g \
 -fsanitize=undefined -fno-sanitize-recover=all "${extra_flags[@]}" \
 -DSTEAMWORKS_TEST_WINDOWS_MAX_MACRO \
 -I"$ROOT_DIR/extension/sdk" -isystem "$SDK_DIR/public/steam" \
 "$ROOT_DIR/tests/typed_boundary_test.cpp" -o "$OUTPUT_DIR/windows_max_macro_test"
"$OUTPUT_DIR/windows_max_macro_test"
echo "PASS: Windows-style max macro remains active across production boundaries"

# Both are expected compile failures. A successful build would weaken the boundary.
cat > "$OUTPUT_DIR/sdk-drift.cpp" <<'CPP'
#include "steam_gameserver.h"
#undef STEAMHTTP_INTERFACE_VERSION
#define STEAMHTTP_INTERFACE_VERSION "STEAMHTTP_INTERFACE_VERSION999"
#include "core/swinterfaces.h"
CPP
if "$CXX" -std=c++14 "${extra_flags[@]}" -I"$ROOT_DIR/extension" -isystem "$SDK_DIR/public/steam" \
 -c "$OUTPUT_DIR/sdk-drift.cpp" -o "$OUTPUT_DIR/sdk-drift.o" > "$OUTPUT_DIR/sdk-drift.log" 2>&1; then
 echo "FAIL: unreviewed SDK version compiled" >&2; exit 1
fi
if ! grep -q 'Steam interface declaration changed' "$OUTPUT_DIR/sdk-drift.log"; then
 cat "$OUTPUT_DIR/sdk-drift.log" >&2; exit 1
fi
cat > "$OUTPUT_DIR/identity-mix.cpp" <<'CPP'
#include "core/swtypes.h"
int main() { steamworks::IndividualIdentity(steamworks::AppId(550)); }
CPP
if "$CXX" -std=c++14 "${extra_flags[@]}" -I"$ROOT_DIR/extension" -isystem "$SDK_DIR/public/steam" \
 -c "$OUTPUT_DIR/identity-mix.cpp" -o "$OUTPUT_DIR/identity-mix.o" > "$OUTPUT_DIR/identity-mix.log" 2>&1; then
 echo "FAIL: AppID implicitly became AccountID" >&2; exit 1
fi
if ! grep -q 'could not convert' "$OUTPUT_DIR/identity-mix.log"; then
 cat "$OUTPUT_DIR/identity-mix.log" >&2; exit 1
fi
echo "PASS: 2 expected compilation rejections (SDK drift and AppID/AccountID mixing)"

# Pin the independent 014 prefix; it is not the current SDK's 015 declaration.
python3 - "$ROOT_DIR" <<'PY'
from pathlib import Path
import json,re,sys
root=Path(sys.argv[1]); record=json.loads((root/'tests/fixtures/gameserver014-prefix.json').read_text())
header=(root/'extension/core/swgameauth014.h').read_text()
declarations=re.findall(r'virtual\s+([^;\n]+?\s*=\s*0)\s*;',header)
normalize=lambda value: re.sub(r'\s+',' ',value).strip()
assert [normalize(v) for v in declarations]==[normalize(v) for v in record['declarations']]
assert len(declarations)==27 and 'BeginAuthSession' in declarations[26]
print('PASS: audited SDK 1.55 auth prefix, 27 ordered slots, independent of SDK 015')
PY
cat > "$OUTPUT_DIR/auth-interface-mix.cpp" <<'CPP'
#include "core/swgameauth014.h"
ISteamGameServer* Mixed(steamworks::ISteamGameServer014* server) { return server; }
CPP
if "$CXX" -std=c++14 "${extra_flags[@]}" -I"$ROOT_DIR/extension" -isystem "$SDK_DIR/public/steam" \
 -c "$OUTPUT_DIR/auth-interface-mix.cpp" -o "$OUTPUT_DIR/auth-interface-mix.o" > "$OUTPUT_DIR/auth-interface-mix.log" 2>&1; then
 echo "FAIL: engine 014 implicitly became SDK 015" >&2; exit 1
fi
if ! grep -Eq 'cannot convert|incompatible.*pointer' "$OUTPUT_DIR/auth-interface-mix.log"; then
 cat "$OUTPUT_DIR/auth-interface-mix.log" >&2; exit 1
fi
echo "PASS: expected compile rejection of auth 014/native 015 mixing"
