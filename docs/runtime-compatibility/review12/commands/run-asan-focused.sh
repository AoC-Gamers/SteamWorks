set -eu
export LD_LIBRARY_PATH=/usr/lib/llvm-14/lib/clang/14.0.6/lib/linux
export ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:alloc_dealloc_mismatch=1:new_delete_type_mismatch=1 LSAN_OPTIONS=exitcode=23 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
mkdir -p .build/review12/asan-focused
flags='-std=c++14 -m32 -O3 -DNDEBUG -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -shared-libasan -fsized-deallocation -fno-pie -no-pie'
for name in hooks_lifecycle_test http_text_response_test; do
 clang++ $flags -Itests/stubs -Iextension/sdk -isystem sdk_158a/public/steam tests/$name.cpp -o .build/review12/asan-focused/$name
 .build/review12/asan-focused/$name
done
