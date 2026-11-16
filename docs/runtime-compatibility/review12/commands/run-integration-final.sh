git config --global --add safe.directory /workspace
set -eu
export SOURCEMOD_DIR=/opt/sm-deps/sourcemod-1.12 MMSOURCE_DIR=/opt/sm-deps/mmsource-1.12
export STEAMWORKS_SDK_DIR=/workspace/sdk_158a
export SPCOMP="${SPCOMP:-/compiler/spcomp64}"
SOURCEPAWN_VM=/workspace/.build/p5-r2/vm-x86/build/libsourcepawn/linux-x86/libsourcepawn.so
export SOURCEPAWN_VM
export VM_TEST_CXX=clang++ VM_TEST_FLAGS='-m32 -fsanitize=address -shared-libasan -fsized-deallocation -fno-pie -no-pie'
export LD_LIBRARY_PATH=/usr/lib/llvm-14/lib/clang/14.0.6/lib/linux
export ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:alloc_dealloc_mismatch=1:new_delete_type_mismatch=1
export LSAN_OPTIONS=exitcode=23 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
python3 scripts/test-integration.py --with-auth-binding --with-auth-forward --with-sourcepawn-environment --output .build/review12/integration-final
