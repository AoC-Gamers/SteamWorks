#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUTPUT_DIR="${TEST_BUILD_DIR:-$ROOT_DIR/.build/buffer-fixture}"
: "${SOURCEMOD_DIR:?Set SOURCEMOD_DIR}"
SDK_DIR="${STEAMWORKS_SDK_DIR:-$ROOT_DIR/sdk_155}"
python3 "$ROOT_DIR/scripts/build/validate-steamworks-sdk.py" "$SDK_DIR"
mkdir -p "$OUTPUT_DIR/sdk"
cp "$ROOT_DIR/extension/sdk/smsdk_ext.h" "$ROOT_DIR/extension/sdk/smsdk_ext.cpp" "$OUTPUT_DIR/sdk/"
cp "$ROOT_DIR/tests/runtime/buffer_fixture_sdk_config.h" "$OUTPUT_DIR/sdk/smsdk_config.h"
cat >> "$OUTPUT_DIR/sdk/smsdk_ext.cpp" <<'CPP'
void operator delete(void* ptr, size_t) noexcept { free(ptr); }
void operator delete[](void* ptr, size_t) noexcept { free(ptr); }
CPP
"${CXX:-g++}" -std=c++14 -m32 -O2 -Wall -Wextra -Werror -Wno-unused-parameter -fPIC -shared -fvisibility=hidden \
 -I"$OUTPUT_DIR/sdk" -I"$ROOT_DIR/extension/core" -I"$SOURCEMOD_DIR/public" \
 -I"$SOURCEMOD_DIR/public/amtl" -I"$SOURCEMOD_DIR/public/amtl/amtl" -I"$SOURCEMOD_DIR/sourcepawn/include" \
 -isystem "$SDK_DIR/public/steam" "$ROOT_DIR/tests/runtime/steamworks_buffer_fixture.cpp" "$OUTPUT_DIR/sdk/smsdk_ext.cpp" \
 -o "$OUTPUT_DIR/steamworks_buffer_fixture.ext.so"
