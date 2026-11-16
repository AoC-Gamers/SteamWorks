set -eu
export SOURCEMOD_DIR=/opt/sm-deps/sourcemod-1.12 MMSOURCE_DIR=/opt/sm-deps/mmsource-1.12 STEAMWORKS_SDK_DIR=/workspace/sdk_158a
export CXX=clang++ TEST_CXXFLAGS='-m32 -fsanitize=address -shared-libasan -fsized-deallocation -fno-pie -no-pie'
export LD_LIBRARY_PATH=/usr/lib/llvm-14/lib/clang/14.0.6/lib/linux
export ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:alloc_dealloc_mismatch=1:new_delete_type_mismatch=1 LSAN_OPTIONS=exitcode=23 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
for name in resource-lifecycle http-stream-context init-coexistence; do
 export TEST_BUILD_DIR=/workspace/.build/auth-ticket-opt/asan-native/$name
 bash scripts/test-$name.sh
done
