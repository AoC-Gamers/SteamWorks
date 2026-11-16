#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT_DIR="${TEST_BUILD_DIR:-$ROOT_DIR/.build/tests-detours}"
CXX="${CXX:-g++}"
SDK_DIR="${STEAMWORKS_SDK_DIR:-$ROOT_DIR/sdk_158a}"
mkdir -p "$OUTPUT_DIR"
if [[ ! -f "$SDK_DIR/public/steam/steam_gameserver.h" ]]; then
  echo "Missing Steamworks SDK headers at $SDK_DIR; set STEAMWORKS_SDK_DIR." >&2
  exit 1
fi

read -r -a extra_flags <<< "${TEST_CXXFLAGS:-}"
"$CXX" -std=c++14 -Wall -Wextra -Werror -pedantic -O1 -g \
  -fsanitize=undefined -fno-sanitize-recover=all "${extra_flags[@]}" \
  -I"$ROOT_DIR/extension/sdk" -isystem "$SDK_DIR/public/steam" \
  "$ROOT_DIR/tests/detours_lifecycle_test.cpp" -o "$OUTPUT_DIR/detours_lifecycle_test"
"$OUTPUT_DIR/detours_lifecycle_test"

# Exercise the fixed-size backup under the same optimized warning policy that
# found the unchecked decoder length. All checks remain active under NDEBUG.
"$CXX" -std=c++14 -Wall -Wextra -Werror -O3 -DNDEBUG -g \
  -fsanitize=undefined -fno-sanitize-recover=all "${extra_flags[@]}" \
  "$ROOT_DIR/tests/cdetour_restore_test.cpp" -o "$OUTPUT_DIR/cdetour_restore_test"
"$OUTPUT_DIR/cdetour_restore_test"
