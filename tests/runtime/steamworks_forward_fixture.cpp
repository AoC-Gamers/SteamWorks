// Laboratory-only driver for the candidate's existing managed forwards.
// It never creates replacement forwards or invokes Steam/engine interfaces.
#include "smsdk_ext.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <vector>

namespace {
const char* names[] = {
 "SW_OnValidateClient", "SteamWorks_OnValidateClient", "SteamWorks_SteamServersConnected",
 "SteamWorks_SteamServersConnectFailure", "SteamWorks_SteamServersDisconnected",
 "SteamWorks_OnClientGroupStatus", "SteamWorks_RestartRequested", "SteamWorks_TokenRequested",
 "SteamWorks_BeginAuthSession", "SteamWorks_GCSendMessage", "SteamWorks_GCMsgAvailable",
 "SteamWorks_GCRetrieveMessage"
};
int active = -1, checks = 0, failures = 0, runs = 0;
std::array<cell_t, 4> returns{};
std::vector<int> seen;
void Check(bool ok, const char* condition, int kind, int scenario) {
 ++checks; if (!ok) ++failures;
 if (!ok) smutils->LogMessage(myself, "[SW-FWD] check=%s kind=%d scenario=%d pass=%d", condition, kind, scenario, ok);
}
cell_t IsActive(IPluginContext*, const cell_t* p) { return active == p[1]; }
cell_t Previous(IPluginContext*, const cell_t*) { return seen.empty() ? 0 : seen.back(); }
cell_t Observe(IPluginContext* context, const cell_t* p) {
 if (active < 0 || p[2] != active || p[1] < 1 || p[1] > 3)
  return context->ThrowNativeError("Forward fixture observation outside active synthetic dispatch");
 seen.push_back(p[1]);
 Check(p[3] != 0, "receiver_arguments", active, -1);
 return returns[p[1]];
}
cell_t Run(IPluginContext* context, const cell_t* p) {
 if (active != -1) return context->ThrowNativeError("Forward fixture cannot run recursively");
 std::vector<int> order;
 int encoded = p[1];
 if (encoded < 0 || encoded > 999) return context->ThrowNativeError("Invalid receiver order");
 for (; encoded; encoded /= 10) order.insert(order.begin(), encoded % 10);
 std::array<bool,4> used{};
 for (int id : order) {
  if (id < 1 || id > 3 || used[id]) return context->ThrowNativeError("Invalid receiver order");
  used[id] = true;
 }
 // Values cover all Action values, GC's legacy -1 override, max aggregation,
 // and Stop in the first/middle receiver. No callback is allowed to reach Steam.
 const int scenarios[][3] = {{0,0,0},{1,1,1},{3,3,3},{4,4,4},{-1,-1,-1},{0,3,1},{4,0,3},{3,4,0}};
 const int before = failures;
 ++runs;
 for (int kind = 0; kind < 12; ++kind) {
  IForward* fwd = forwards->FindForward(names[kind], nullptr);
  Check(fwd != nullptr, "existing_forward", kind, -1);
  if (!fwd) continue;
  const ExecType mode = kind == 6 ? ET_Hook : ((kind == 9 || kind == 11) ? ET_Event : ET_Ignore);
  Check(fwd->GetExecType() == mode, "execution_type", kind, -1);
  Check(fwd->GetFunctionCount() == order.size(), "receiver_count", kind, -1);
  if (fwd->GetFunctionCount() != order.size()) continue;
  for (int scenario = 0; scenario < 8; ++scenario) {
   returns.fill(0);
   for (size_t i = 0; i < order.size(); ++i) returns[order[i]] = scenarios[scenario][i];
   unsigned char bytes[8] = {0xa5,0x00,0x80,0x7f,0x42,0x00,0xfe,0x00};
   cell_t cells[2]; std::memcpy(cells, bytes, sizeof(cells));
   char token[32] = "synthetic-seed";
   int pushed = SP_ERROR_NONE;
   auto cell = [&](cell_t value) { pushed |= fwd->PushCell(value); };
   switch (kind) {
    case 0: case 1: cell(static_cast<cell_t>(0xf1234567u)); cell(0x12345678); break;
    case 2: case 6: break;
    case 3: case 4: cell(17); break;
    case 5: cell(static_cast<cell_t>(0xf1234567u)); cell(0x12345678); cell(1); cell(0); break;
    case 7: pushed |= fwd->PushStringEx(token, sizeof(token), SM_PARAM_STRING_UTF8 | SM_PARAM_STRING_COPY, SM_PARAM_COPYBACK); cell(sizeof(token)); break;
    case 8: pushed |= fwd->PushArray(cells, 2, 0); cell(7); cell(static_cast<cell_t>(0xf1234567u)); break;
    case 9: cell(static_cast<cell_t>(0x80000042u)); pushed |= fwd->PushStringEx(reinterpret_cast<char*>(bytes), 7, SM_PARAM_STRING_BINARY | SM_PARAM_STRING_COPY, 0); cell(7); break;
    case 10: cell(7); break;
    case 11: cell(static_cast<cell_t>(0x80000042u)); pushed |= fwd->PushStringEx(reinterpret_cast<char*>(bytes), 7, SM_PARAM_STRING_BINARY | SM_PARAM_STRING_COPY, 0); cell(7); cell(5); break;
   }
   Check(pushed == SP_ERROR_NONE, "push_parameters", kind, scenario);
   if (pushed != SP_ERROR_NONE) { fwd->Cancel(); continue; }
   seen.clear(); active = kind;
   cell_t result = 0x12341234;
   int error = fwd->Execute(&result);
   active = -1;
   std::vector<int> expected;
   cell_t aggregate = 0;
   for (int id : order) {
    expected.push_back(id);
    aggregate = std::max(aggregate, returns[id]);
    if (mode == ET_Hook && aggregate == Pl_Stop) break;
   }
   cell_t expectedResult = order.empty() ? 0x12341234 : (mode == ET_Ignore ? 0 : aggregate);
   Check(error == SP_ERROR_NONE, "dispatch_status_not_callback_result", kind, scenario);
   Check(seen == expected, "order_count_and_stop", kind, scenario);
   Check(result == expectedResult, "aggregated_result", kind, scenario);
   const unsigned char original[8] = {0xa5,0x00,0x80,0x7f,0x42,0x00,0xfe,0x00};
   Check(std::memcmp(cells, original, 8) == 0 && std::memcmp(bytes, original, 8) == 0, "binary_no_copyback", kind, scenario);
   smutils->LogMessage(myself, "[SW-FWD] dispatch kind=%d scenario=%d receivers=%u callbacks=%u result=%d expected=%d error=%d", kind, scenario, static_cast<unsigned>(order.size()), static_cast<unsigned>(seen.size()), result, expectedResult, error);
   if (kind == 7) {
    char expectedToken[32] = "synthetic-seed";
    if (!expected.empty()) ke::SafeSprintf(expectedToken, sizeof(expectedToken), "synthetic-%d", expected.back());
    Check(std::strcmp(token, expectedToken) == 0, "token_copyback", kind, scenario);
   }
  }
 }
 smutils->LogMessage(myself, "[SW-FWD] summary runs=%d order=%d checks=%d failures=%d active=%d", runs, p[1], checks, failures, active);
 return failures == before;
}
const sp_nativeinfo_t natives[] = {
 {"SWForwardFixture_IsActive", IsActive}, {"SWForwardFixture_Previous", Previous},
 {"SWForwardFixture_Observe", Observe}, {"SWForwardFixture_Run", Run}, {nullptr,nullptr}
};
class Fixture final : public SDKExtension {
 bool SDK_OnLoad(char*, size_t, bool) override {
  sharesys->AddNatives(myself, natives);
  smutils->LogMessage(myself, "[SW-FWD] fixture_loaded synthetic_only=1 replacement_forwards=0");
  return true;
 }
 void SDK_OnUnload() override { active = -1; seen.clear(); }
} fixture;
}
SMEXT_LINK(&fixture);
