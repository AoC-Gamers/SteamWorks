#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUTPUT_DIR="${TEST_BUILD_DIR:-$ROOT_DIR/.build/tests-auth-binding}"
SDK_DIR="${STEAMWORKS_SDK_DIR:-$ROOT_DIR/sdk_155}"
python3 "$ROOT_DIR/scripts/build/validate-steamworks-sdk.py" "$SDK_DIR"
: "${MMSOURCE_DIR:?Set MMSOURCE_DIR to the audited Metamod checkout}"
SH_DIR="$MMSOURCE_DIR/core/sourcehook"
mkdir -p "$OUTPUT_DIR"
python3 - "$SH_DIR" "$OUTPUT_DIR" <<'PYCODE'
from pathlib import Path
import hashlib,json,sys
source,out=map(Path,sys.argv[1:])
(out/'sourcehook-inputs.json').write_text(json.dumps({p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(source.glob('*.h'))+sorted(source.glob('*.cpp'))},indent=2)+'\n')
PYCODE
CXX="${AUTH_BINDING_CXX:-${CXX:-g++}}"
virtual_flags=()
if ! "$CXX" --version | grep -qi clang; then virtual_flags=(-fno-devirtualize); fi
read -r -a extra_flags <<< "${TEST_CXXFLAGS:--m32}"
"$CXX" -std=c++14 -Wall -Wextra -Werror -Wno-unused-parameter -O1 -g \
 -fsanitize=address,undefined -fno-sanitize-recover=all "${extra_flags[@]}" \
 -isystem "$SH_DIR" "$ROOT_DIR/tests/integration/sourcehook_member_pointer_test.cpp" \
 -o "$OUTPUT_DIR/sourcehook_member_pointer_test"
"$OUTPUT_DIR/sourcehook_member_pointer_test"
# Build the pinned third-party implementation separately using its default
# warning policy; strict diagnostics below apply to our owner, adapter and test.
objects=()
for source in sourcehook sourcehook_impl_chookmaninfo sourcehook_impl_chookidman sourcehook_impl_cproto sourcehook_impl_cvfnptr; do
 object="$OUTPUT_DIR/$source.o"
 "$CXX" -std=c++14 -O1 -g -fsanitize=undefined -fno-sanitize-recover=all "${extra_flags[@]}" \
  -isystem "$SH_DIR" -c "$SH_DIR/$source.cpp" -o "$object"
 objects+=("$object")
done
"$CXX" -std=c++14 -Wall -Wextra -Werror -Wno-unused-parameter -O1 -g \
 "${virtual_flags[@]}" -fsanitize=undefined -fno-sanitize-recover=all "${extra_flags[@]}" \
 -I"$ROOT_DIR/extension/hooks" -I"$ROOT_DIR/tests/support" -I"$ROOT_DIR/extension/sdk" -isystem "$SDK_DIR/public/steam" -isystem "$SH_DIR" \
 "$ROOT_DIR/tests/integration/auth_binding_test.cpp" "${objects[@]}" -o "$OUTPUT_DIR/auth_binding_test"
"$OUTPUT_DIR/auth_binding_test"

# Negative control: restoring the wrong SDK-object hook must lose the engine event.
python3 - "$ROOT_DIR" "$OUTPUT_DIR" <<'PYCODE'
from pathlib import Path
import sys
root,out=map(Path,sys.argv[1:]); source=(root/'extension/hooks/swgshooks.cpp').read_text()
a='SH_DECL_HOOK3(ISteamGameServer014, BeginAuthSession,'
b='SH_ADD_HOOK(ISteamGameServer014, BeginAuthSession, authTarget,'
assert source.count(a)==1 and source.count(b)==1
source=source.replace(a,'SH_DECL_HOOK3(ISteamGameServer, BeginAuthSession,').replace(b,'SH_ADD_HOOK(ISteamGameServer, BeginAuthSession, target,')
(out/'old-target.cpp').write_text(source)
PYCODE
"$CXX" -std=c++14 -Wall -Wextra -Werror -Wno-unused-parameter -O1 -g \
 "${virtual_flags[@]}" -fsanitize=undefined -fno-sanitize-recover=all "${extra_flags[@]}" \
 -I"$ROOT_DIR/extension/hooks" -I"$ROOT_DIR/tests/support" -I"$ROOT_DIR/extension/sdk" -isystem "$SDK_DIR/public/steam" -isystem "$SH_DIR" \
 "-DAUTH_BINDING_HOOK_SOURCE=\"$OUTPUT_DIR/old-target.cpp\"" \
 "$ROOT_DIR/tests/integration/auth_binding_test.cpp" "${objects[@]}" -o "$OUTPUT_DIR/old_target_test"
if "$OUTPUT_DIR/old_target_test" > "$OUTPUT_DIR/old-target.log" 2>&1; then
 echo "FAIL: restoring the wrong SDK-object hook was not detected" >&2; exit 1
fi
grep -q 'FAIL: real SourceHook delivers one forward and one original call' "$OUTPUT_DIR/old-target.log"
if grep -Eq 'AddressSanitizer:|LeakSanitizer:|runtime error:' "$OUTPUT_DIR/old-target.log"; then
 cat "$OUTPUT_DIR/old-target.log" >&2; exit 1
fi
echo "PASS: negative control rejects the wrong SDK-object hook target"
