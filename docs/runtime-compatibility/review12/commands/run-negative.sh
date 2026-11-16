set -eu
export STEAMWORKS_SDK_DIR=/workspace/sdk_158a
mkdir -p .build/review12/negative
common='-std=c++14 -m32 -Wall -Wextra -Werror -pedantic -O1 -g -fsanitize=undefined -fno-sanitize-recover=all'
g++ $common -Itests/stubs -Iextension/sdk -Iextension/hooks -Iextension/http -isystem sdk_158a/public/steam -DGC_HOOK_SOURCE=\"/workspace/.build/review12/source-before/swgchooks.cpp\" tests/hooks_lifecycle_test.cpp -o .build/review12/negative/gc
if .build/review12/negative/gc > .build/review12/negative/gc.log 2>&1; then echo 'GC regression did not fail'; exit 1; fi
grep -q 'FAIL: GC payload tail contains only initialized zeros' .build/review12/negative/gc.log
g++ $common -Itests/stubs -Iextension/sdk -Iextension/hooks -Iextension/http -isystem sdk_158a/public/steam -DHTTP_REQUEST_SOURCE=\"/workspace/.build/review12/source-before/swhttprequest.cpp\" tests/http_text_response_test.cpp -o .build/review12/negative/http
if .build/review12/negative/http > .build/review12/negative/http.log 2>&1; then echo 'HTTP regression did not fail'; exit 1; fi
grep -q 'FAIL: strict HTTP truncation reads full provider body' .build/review12/negative/http.log
echo 'PASS: both representative regressions fail against unchanged production code'
