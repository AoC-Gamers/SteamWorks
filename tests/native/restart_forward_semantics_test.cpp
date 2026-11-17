// Reuse the established SDK/SourceHook doubles and compile the real hook body.
// The live CForward aggregation is exercised separately in SRCDS.
#define main ExistingLifecycleFixtureMain
#include "hooks_lifecycle_test.cpp"
#undef main

int main()
{
 resolver.server = &server1; resolver.auth = &auth1; resolver.gc = &gc1;
 {
  SteamWorksGSHooks owner;
  auto* restart = forwards->owned.at("SteamWorks_RestartRequested");
  for (bool original : {false, true}) {
   server1.restart = original;
   for (int receivers : {0, 1, 3}) {
    restart->consumers = receivers;
    for (cell_t aggregate : {-1, 0, 1, 2, 3, 4}) {
     restart->result = aggregate;
     int calls = restart->executions, originals = server1.restarts;
     const bool actual = owner.WasRestartRequested();
     Check(actual == ((original && receivers != 0) ? aggregate >= 3 : original),
      "historical SDK boolean convention: handled/stop retain true, continue/changed suppress");
     Check(restart->executions == calls + (original && receivers != 0 ? 1 : 0),
      "false SDK result and zero consumers never execute restart forward");
     Check(server1.restarts == originals + 1 && meta == MRES_SUPERCEDE,
      "original SDK queried exactly once; return replaces query, not an actual restart command");
    }
   }
  }
 }
 Check(hooks.empty() && host.frames.empty() && forwards->owned.empty(), "all fixture hooks/forwards released");
 std::printf("PASS: %d restart-forward semantic checks (production hook; synthetic SDK/forward/SourceHook)\n", checks);
}
