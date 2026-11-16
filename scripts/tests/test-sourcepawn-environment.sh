#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
: "${SOURCEMOD_DIR:?Set SOURCEMOD_DIR to the headers checkout}"
: "${SOURCEPAWN_VM:?Set SOURCEPAWN_VM to the isolated, instrumented VM}"
OUTPUT_DIR="${TEST_BUILD_DIR:-$ROOT_DIR/.build/tests-sourcepawn-environment}"
mkdir -p "$OUTPUT_DIR"
read -r -a extra_flags <<< "${TEST_CXXFLAGS:-}"
"${CXX:-g++}" -std=c++14 -Wall -Wextra -Werror -O1 -g \
 -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
 "${extra_flags[@]}" -isystem "$SOURCEMOD_DIR/sourcepawn/include" \
 "$ROOT_DIR/tests/integration/sourcepawn_environment_test.cpp" -ldl -o "$OUTPUT_DIR/sourcepawn_environment_test"
"$OUTPUT_DIR/sourcepawn_environment_test" "$SOURCEPAWN_VM"
