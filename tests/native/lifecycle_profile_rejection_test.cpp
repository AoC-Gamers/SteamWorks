// Constructor uses the actual production policy, not the simulated profile used
// by factory/lifecycle unit tests. Unknown binaries must never be resolved/patched.
#define TEST_REAL_LIFECYCLE_PROFILE_POLICY
#include "detours_test_host.h"
int main()
{
 (void)InvokeInit;
 ResetFixture();
 {
  SteamWorksGSDetours owner;
  Require(!owner.IsInstalled(), "unverified module pair cannot enable lifecycle detours");
  Require(detourState.libraryOpens==0 && detourState.exportReads==0 && detourState.creates==0 && detourState.enables==0,
    "missing binary profile rejected before module resolution or detour factory");
  Require(detourState.errors.size()==1 && detourState.errors[0].find("binary profile")!=std::string::npos,
    "actionable unknown profile diagnostic");
  owner.Shutdown();owner.Shutdown();
 }
 Require(detourState.destroys==0 && detourState.live==0, "rejected profile has no owned patches");
 std::puts("PASS: lifecycle profile rejection before any symbol lookup or patch (actual production policy)");
}
