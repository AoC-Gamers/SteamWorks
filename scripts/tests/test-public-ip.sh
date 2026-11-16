#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUTPUT_DIR="${TEST_BUILD_DIR:-$ROOT_DIR/.build/tests-public-ip}"
CXX="${CXX:-g++}"
SDK_DIR="${STEAMWORKS_SDK_DIR:-$ROOT_DIR/sdk_158a}"
mkdir -p "$OUTPUT_DIR"
if [[ ! -f "$SDK_DIR/public/steam/steamclientpublic.h" ]]; then
  echo "Missing Steamworks SDK headers at $SDK_DIR; set STEAMWORKS_SDK_DIR." >&2
  exit 1
fi

read -r -a extra_flags <<< "${TEST_CXXFLAGS:-}"
for char_mode in signed unsigned; do
  # The included production file has unrelated legacy natives with unused args.
  "$CXX" -std=c++14 -Wall -Wextra -Werror -Wno-unused-parameter -pedantic -O1 -g \
    -fsanitize=undefined -fno-sanitize-recover=all "${extra_flags[@]}" "-f${char_mode}-char" \
    -I"$ROOT_DIR/tests/support" -I"$ROOT_DIR/extension/sdk" -isystem "$SDK_DIR/public/steam" \
    "$ROOT_DIR/tests/native/public_ip_test.cpp" -o "$OUTPUT_DIR/public_ip_test_$char_mode"
  "$OUTPUT_DIR/public_ip_test_$char_mode"
done
