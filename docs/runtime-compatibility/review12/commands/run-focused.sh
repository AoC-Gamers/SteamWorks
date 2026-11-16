set -eu
export TEST_CXXFLAGS=-m32
export TEST_BUILD_DIR=/workspace/.build/review12/focused-typed
bash scripts/test-typed-boundary.sh
export TEST_BUILD_DIR=/workspace/.build/review12/focused-lifecycle
bash scripts/test-resource-lifecycle.sh
