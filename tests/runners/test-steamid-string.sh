#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUTPUT_DIR="${TEST_BUILD_DIR:-$ROOT_DIR/.build/tests-steamid-string}"
CXX="${CXX:-g++}"
SDK_DIR="${STEAMWORKS_SDK_DIR:-$ROOT_DIR/sdk_155}"
python3 "$ROOT_DIR/scripts/build/validate-steamworks-sdk.py" "$SDK_DIR"
AMTL_DIR="${AMTL_DIR:-${SOURCEMOD_DIR:-$ROOT_DIR/.deps/exts-linux/sourcemod-1.12}/public/amtl}"
mkdir -p "$OUTPUT_DIR"
if [[ ! -f "$SDK_DIR/public/steam/steamclientpublic.h" || ! -f "$AMTL_DIR/amtl/am-string.h" ]]; then
  echo "Missing SDK or AMTL headers; set STEAMWORKS_SDK_DIR and AMTL_DIR (SourceMod public/amtl directory)." >&2
  exit 1
fi

read -r -a extra_flags <<< "${TEST_CXXFLAGS:-}"
"$CXX" -std=c++14 -Wall -Wextra -Werror -Wno-unused-parameter -pedantic -O1 -g \
  -fsanitize=undefined -fno-sanitize-recover=all "${extra_flags[@]}" \
  -I"$ROOT_DIR/tests/support" -I"$ROOT_DIR/extension/sdk" -isystem "$SDK_DIR/public/steam" -isystem "$AMTL_DIR" \
  "$ROOT_DIR/tests/native/steamid_string_test.cpp" -o "$OUTPUT_DIR/steamid_string_test"
"$OUTPUT_DIR/steamid_string_test"
