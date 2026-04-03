#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEPS_DIR="${DEPS_DIR:-$ROOT_DIR/.deps}"
TARGET_SDK="${TARGET_SDK:-l4d2}"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/.build/linux-$TARGET_SDK}"
SOURCEMOD_DIR="${SOURCEMOD_DIR:-$DEPS_DIR/sourcemod-1.12}"
MMSOURCE_DIR="${MMSOURCE_DIR:-$DEPS_DIR/mmsource-1.12}"
STEAMWORKS_SDK_NAME="${STEAMWORKS_SDK_NAME:-sdk}"
STEAMWORKS_SDK_DIR="${STEAMWORKS_SDK_DIR:-$ROOT_DIR/$STEAMWORKS_SDK_NAME}"
VENV_DIR="${VENV_DIR:-$DEPS_DIR/.venv-linux}"
CONFIGURE_SCRIPT="${CONFIGURE_SCRIPT:-$ROOT_DIR/configure.py}"

case "$(uname -s)" in
  Linux)
    ;;
  *)
    echo "This build script targets Linux L4D2 extensions and must be run on Linux." >&2
    echo "Use make deps on any platform, but run make build inside a Linux environment with 32-bit toolchain support." >&2
    exit 1
    ;;
esac

case "$TARGET_SDK" in
  l4d2)
    HL2SDK_DIR="${HL2SDK_DIR:-$DEPS_DIR/hl2sdk-l4d2}"
    ;;
  sdk2013)
    HL2SDK_DIR="${HL2SDK_DIR:-$DEPS_DIR/hl2sdk-sdk2013}"
    ;;
  *)
    echo "Unsupported TARGET_SDK=$TARGET_SDK. Expected l4d2 or sdk2013." >&2
    exit 1
    ;;
esac

venv_python() {
  if [[ -x "$VENV_DIR/bin/python" ]]; then
    printf '%s\n' "$VENV_DIR/bin/python"
    return
  fi

  if [[ -x "$VENV_DIR/Scripts/python.exe" ]]; then
    printf '%s\n' "$VENV_DIR/Scripts/python.exe"
    return
  fi

  return 1
}

venv_ambuild() {
  if [[ -x "$VENV_DIR/bin/ambuild" ]]; then
    printf '%s\n' "$VENV_DIR/bin/ambuild"
    return
  fi

  if [[ -x "$VENV_DIR/Scripts/ambuild.exe" ]]; then
    printf '%s\n' "$VENV_DIR/Scripts/ambuild.exe"
    return
  fi

  if [[ -x "$VENV_DIR/Scripts/ambuild" ]]; then
    printf '%s\n' "$VENV_DIR/Scripts/ambuild"
    return
  fi

  return 1
}

if ! VENV_PYTHON="$(venv_python)"; then
  echo "Missing Python venv at $VENV_DIR. Run scripts/fetch-linux-deps.sh first." >&2
  exit 1
fi

if ! VENV_AMBUILD="$(venv_ambuild)"; then
  echo "Missing AMBuild in $VENV_DIR. Run scripts/fetch-linux-deps.sh first." >&2
  exit 1
fi

for required_dir in "$HL2SDK_DIR" "$SOURCEMOD_DIR" "$MMSOURCE_DIR" "$STEAMWORKS_SDK_DIR"; do
  if [[ ! -d "$required_dir" ]]; then
    echo "Missing required directory: $required_dir" >&2
    exit 1
  fi
done

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

pushd "$BUILD_DIR" > /dev/null
"$VENV_PYTHON" "$CONFIGURE_SCRIPT" \
  --sdks "$TARGET_SDK" \
  --hl2sdk-root "$DEPS_DIR" \
  --mms-path "$MMSOURCE_DIR" \
  --sm-path "$SOURCEMOD_DIR" \
  --steamworks-path "$STEAMWORKS_SDK_DIR"

if [[ ! -f "$BUILD_DIR/.ambuild2/vars" ]]; then
  echo "AMBuild configure did not produce $BUILD_DIR/.ambuild2/vars." >&2
  echo "Check the configure output above for the real error." >&2
  exit 1
fi

"$VENV_AMBUILD"
popd > /dev/null

PACKAGE_DIR="$BUILD_DIR/package/addons/sourcemod/extensions"
EXT_BIN="$PACKAGE_DIR/SteamWorks.ext.so"

if [[ ! -f "$EXT_BIN" ]]; then
  echo "Build completed but $EXT_BIN was not found." >&2
  exit 1
fi

cat <<EOF
Build complete.
BUILD_DIR=$BUILD_DIR
EXTENSION=$EXT_BIN
EOF
