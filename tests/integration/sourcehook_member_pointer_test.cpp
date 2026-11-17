// Pin the actual third-party macros behind Sonar's objectIndex reports.
#include "sourcehook.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

struct Left { int left; };
struct Right { int right; void Method() {} };
struct Combined : Left, Right {};
using Member = void (Combined::*)();
static_assert(sizeof(void*) == 4, "SourceHook member-pointer regression requires x86");
static_assert(sizeof(Member) == 2 * sizeof(void*), "Audited GNU member-pointer ABI has two words");
int checks = 0;
void Check(bool ok, const char* why) {
    ++checks;
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", why); std::exit(1); }
}
struct Guarded {
    uint32_t before = 0x12345678;
    Member mfp = static_cast<Member>(&Right::Method);
    uint32_t after = 0x87654321;
};
int main() {
    Guarded guarded;
    uintptr_t original[2]; std::memcpy(original, &guarded.mfp, sizeof(original));
    Check(original[1] == sizeof(Left), "real multiple inheritance creates a nonzero adjustment word");
    SH__MFP_ZERO_HIGH_WORD(guarded.mfp);
    uintptr_t cleared[2]; std::memcpy(cleared, &guarded.mfp, sizeof(cleared));
    Check(cleared[0] == original[0] && cleared[1] == 0, "high-word macro writes within the two-word member pointer");
    Check(guarded.before == 0x12345678 && guarded.after == 0x87654321, "high-word macro preserves adjacent canaries");
    guarded.mfp = static_cast<Member>(&Right::Method);
    void* vfnptr_origentry = nullptr;
    // This macro appears in the generated hook classes. A null target gives a
    // valid null member-pointer representation; no forged target is invoked.
    SH_SETUP_MFP(guarded.mfp);
    uintptr_t setup[2]; std::memcpy(setup, &guarded.mfp, sizeof(setup));
    Check(setup[0] == 0 && setup[1] == 0, "setup macro initializes both words within the member-pointer object");
    Check(guarded.before == 0x12345678 && guarded.after == 0x87654321, "setup macro preserves adjacent canaries");
    std::printf("PASS: %d actual SourceHook member-pointer ABI checks (ASan/UBSan x86)\n", checks);
}
