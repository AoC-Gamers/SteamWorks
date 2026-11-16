#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT_DIR="${TEST_BUILD_DIR:-$ROOT_DIR/.build/restart-forward-semantics}"
SDK_DIR="${STEAMWORKS_SDK_DIR:-$ROOT_DIR/sdk_158a}"
mkdir -p "$OUTPUT_DIR"
read -r -a extra_flags <<< "${TEST_CXXFLAGS:--m32}"
"${CXX:-g++}" -std=c++14 -Wall -Wextra -Werror -pedantic -O3 -DNDEBUG -g \
 -fsanitize=undefined -fno-sanitize-recover=all "${extra_flags[@]}" \
 -I"$ROOT_DIR/tests/stubs" -I"$ROOT_DIR/extension/sdk" -isystem "$SDK_DIR/public/steam" \
 "$ROOT_DIR/tests/restart_forward_semantics_test.cpp" -o "$OUTPUT_DIR/restart_forward_semantics_test"
"$OUTPUT_DIR/restart_forward_semantics_test"
