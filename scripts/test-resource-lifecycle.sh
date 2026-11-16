#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT_DIR="${TEST_BUILD_DIR:-$ROOT_DIR/.build/tests-lifecycle}"
SDK_DIR="${STEAMWORKS_SDK_DIR:-$ROOT_DIR/sdk_158a}"
CXX="${CXX:-g++}"
mkdir -p "$OUTPUT_DIR"
read -r -a extra_flags <<< "${TEST_CXXFLAGS:-}"
for suite in client_lifecycle_test http_lifecycle_test forwards_lifecycle_test hooks_lifecycle_test runtime_lifecycle_test lifecycle_profile_rejection_test lifecycle_profile_identity_test; do
 optimize_flags=()
 # Keep the production authentication allocation checked under the optimizer
 # that exposed its x86 padded-object overflow; Check() remains active in NDEBUG.
 if [[ "$suite" == hooks_lifecycle_test ]]; then optimize_flags=(-O3 -DNDEBUG); fi
 "$CXX" -std=c++14 -Wall -Wextra -Werror -pedantic -O1 -g \
  -fsanitize=undefined -fno-sanitize-recover=all "${extra_flags[@]}" "${optimize_flags[@]}" \
  -I"$ROOT_DIR/tests/stubs" -I"$ROOT_DIR/extension/sdk" -isystem "$SDK_DIR/public/steam" \
  "$ROOT_DIR/tests/$suite.cpp" -o "$OUTPUT_DIR/$suite"
 "$OUTPUT_DIR/$suite"
done
