#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUTPUT_DIR="${TEST_BUILD_DIR:-$ROOT_DIR/.build/tests-debug-log}"
mkdir -p "$OUTPUT_DIR"
read -r -a extra_flags <<< "${TEST_CXXFLAGS:-}"
"${CXX:-g++}" -std=c++14 -Wall -Wextra -Werror -pedantic -O1 -g \
 -fsanitize=undefined -fno-sanitize-recover=all "${extra_flags[@]}" \
 "$ROOT_DIR/tests/native/debug_log_test.cpp" -o "$OUTPUT_DIR/debug_log_test"
"$OUTPUT_DIR/debug_log_test"
