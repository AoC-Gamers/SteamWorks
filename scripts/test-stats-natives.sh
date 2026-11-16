#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT_DIR="${TEST_BUILD_DIR:-$ROOT_DIR/.build/tests-stats}"
CXX="${CXX:-g++}"
SDK_DIR="${STEAMWORKS_SDK_DIR:-$ROOT_DIR/sdk_158a}"
mkdir -p "$OUTPUT_DIR"
if [[ ! -f "$SDK_DIR/public/steam/steamclientpublic.h" ]]; then
  echo "Missing Steamworks SDK headers at $SDK_DIR; set STEAMWORKS_SDK_DIR." >&2
  exit 1
fi

# Optional flags allow the same fixture to run with -m32 inside a multilib
# environment. This suite deliberately does not link against a Steam runtime.
read -r -a extra_flags <<< "${TEST_CXXFLAGS:-}"
for suite in stats_natives_test stats_requests_test; do
  "$CXX" -std=c++14 -Wall -Wextra -Werror -pedantic -O1 -g \
    -fsanitize=undefined -fno-sanitize-recover=all "${extra_flags[@]}" \
    -I"$ROOT_DIR/extension/sdk" -isystem "$SDK_DIR/public/steam" \
    "$ROOT_DIR/tests/$suite.cpp" -o "$OUTPUT_DIR/$suite"
  "$OUTPUT_DIR/$suite"
done
