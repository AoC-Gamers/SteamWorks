#!/usr/bin/env bash
# Laboratory-only x86 extension; never installs or launches a server.
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
OUTPUT_DIR="${TEST_BUILD_DIR:-$ROOT_DIR/.build/forward-fixture}"
: "${SOURCEMOD_DIR:?Set SOURCEMOD_DIR to a compatible SourceMod source tree}"
mkdir -p "$OUTPUT_DIR/sdk"
cp "$ROOT_DIR/extension/sdk/smsdk_ext.h" "$OUTPUT_DIR/sdk/"
cp "$ROOT_DIR/extension/sdk/smsdk_ext.cpp" "$OUTPUT_DIR/sdk/"
cp "$ROOT_DIR/tests/runtime/forward_fixture_sdk_config.h" "$OUTPUT_DIR/sdk/smsdk_config.h"
# The old SDK base provides malloc-backed unsized operators. Complete the same
# allocator pairing for C++14 in this local diagnostic copy, not in the product.
cat >> "$OUTPUT_DIR/sdk/smsdk_ext.cpp" <<'CPP'

void operator delete(void* ptr, size_t) noexcept { free(ptr); }
void operator delete[](void* ptr, size_t) noexcept { free(ptr); }
CPP
"${CXX:-g++}" -std=c++14 -m32 -O2 -Wall -Wextra -Werror -Wno-unused-parameter \
 -fPIC -shared -fvisibility=hidden -I"$OUTPUT_DIR/sdk" -I"$SOURCEMOD_DIR/public" \
 -I"$SOURCEMOD_DIR/public/amtl" -I"$SOURCEMOD_DIR/public/amtl/amtl" \
 -I"$SOURCEMOD_DIR/sourcepawn/include" \
 "$ROOT_DIR/tests/runtime/steamworks_forward_fixture.cpp" "$OUTPUT_DIR/sdk/smsdk_ext.cpp" \
 -o "$OUTPUT_DIR/steamworks_forward_fixture.ext.so"
