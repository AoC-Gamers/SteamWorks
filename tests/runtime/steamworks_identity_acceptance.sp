#pragma semicolon 1
#pragma newdecls required
#include <sourcemod>
#include <steamworks>

public Plugin myinfo = {name = "SteamWorks Identity Acceptance", author = "Lechuga", version = "1.0.0"};

int g_Checks, g_Failures, g_ExpectedErrors, g_FixtureSerial, g_FixtureSlot;
int g_Preauth, g_Disconnected;
int g_PreauthSerial[MAXPLAYERS + 1];
bool g_Returned;
bool g_StatsAccepted;
char g_Buffer[48];

public void OnPluginStart()
{
 RegAdminCmd("sm_sw_identity", CheckIdentity, ADMFLAG_ROOT);
 PrintToServer("[SW-IDENT] ready");
}
public void OnPluginEnd()
{
 int client = GetClientFromSerial(g_FixtureSerial);
 if (client > 0 && IsClientConnected(client) && IsFakeClient(client)) KickClient(client, "Identity fixture cleanup");
}
void Check(bool passed, const char[] name)
{
 g_Checks++;
 if (!passed) g_Failures++;
 PrintToServer("[SW-IDENT] check=%s pass=%d", name, passed);
}
void Fill() { for (int i = 0; i < sizeof(g_Buffer); i++) g_Buffer[i] = 'Z'; }
bool Unchanged()
{
 for (int i = 0; i < sizeof(g_Buffer); i++) if (g_Buffer[i] != 'Z') return false;
 return true;
}
public void BufferCall(int client, int capacity)
{
 SteamWorks_GetClientSteamID(client, g_Buffer[4], capacity);
 g_Returned = true;
}
public void AddressCall(int client, int capacity, char[] output)
{
 SteamWorks_GetClientSteamID(client, output, capacity);
 g_Returned = true;
}
public void StatsCall(int client)
{
 g_StatsAccepted = SteamWorks_RequestStats(client, 550);
 g_Returned = true;
}
void ErrorCase(int client, int capacity, int expected, const char[] name, bool invalidAddress = false)
{
 Fill(); g_Returned = false;
 if (invalidAddress) Call_StartFunction(GetMyHandle(), AddressCall);
 else Call_StartFunction(GetMyHandle(), BufferCall);
 Call_PushCell(client); Call_PushCell(capacity);
 // A deliberately invalid VM address, forwarded without dereferencing by the
 // wrapper. The production native must validate it through LocalToString.
 if (invalidAddress) Call_PushCell(0x7fffffff);
 int error = Call_Finish();
 bool ok = error == expected && (expected == SP_ERROR_NONE ? g_Returned : !g_Returned) && Unchanged();
 if (expected != SP_ERROR_NONE && error == expected) g_ExpectedErrors++;
 Check(ok, name);
 PrintToServer("[SW-IDENT] native_error expected=%d observed=%d unchanged=%d returned=%d", expected, error, Unchanged(), g_Returned);
}
void Buffers(int client, bool human)
{
 char full[32], validated[32];
 int count = SteamWorks_GetClientSteamID(client, full, sizeof(full));
 int length = strlen(full);
 Check(count == length + 1 && length > 0, "full_count_includes_NUL");
 if (human) Check(GetClientAuthId(client, AuthId_SteamID64, validated, sizeof(validated), true) && StrEqual(full, validated), "human_matches_SourceMod");
 else Check(StrEqual(full, "76561197960265728"), "bot_public_zero_account_fallback");
 int capacities[7]; capacities[0] = 32; capacities[1] = length + 1; capacities[2] = length;
 capacities[3] = 5; capacities[4] = 2; capacities[5] = 1; capacities[6] = 0;
 for (int test = 0; test < sizeof(capacities); test++)
 {
  int capacity = capacities[test]; Fill();
  int stored = SteamWorks_GetClientSteamID(client, g_Buffer[4], capacity);
  bool ok = capacity == 0 ? stored == 0 && Unchanged() : true;
  if (capacity > 0)
  {
   int copied = capacity - 1 < length ? capacity - 1 : length;
   ok = stored == copied + 1 && g_Buffer[4 + copied] == 0;
   for (int i = 0; i < copied; i++) if (g_Buffer[4 + i] != full[i]) ok = false;
   for (int i = 0; i < sizeof(g_Buffer); i++) if ((i < 4 || i >= 4 + capacity) && g_Buffer[i] != 'Z') ok = false;
  }
  Check(ok, "buffer_capacity_count_NUL_canaries");
  PrintToServer("[SW-IDENT] buffer human=%d capacity_case=%d pass=%d", human, test, ok);
 }
 ErrorCase(client, -1, SP_ERROR_NATIVE, "negative_capacity");
 ErrorCase(client, view_as<int>(0x80000000), SP_ERROR_NATIVE, "minimum_signed_capacity");
 ErrorCase(client, 18, SP_ERROR_INVALID_ADDRESS, "invalid_output_address", true);
 ErrorCase(client, 0, SP_ERROR_NONE, "zero_before_invalid_address", true);
 ErrorCase(client, -1, SP_ERROR_NATIVE, "negative_before_invalid_address", true);
}
public void OnClientConnected(int client)
{
 if (IsFakeClient(client) || IsClientAuthorized(client)) return;
 g_PreauthSerial[client] = GetClientSerial(client);
 g_Preauth++;
 g_Returned = false; g_StatsAccepted = true;
 Call_StartFunction(GetMyHandle(), StatsCall); Call_PushCell(client);
 int error = Call_Finish();
 if (error == SP_ERROR_NATIVE) g_ExpectedErrors++;
 Check((error == SP_ERROR_NATIVE && !g_Returned) || (error == SP_ERROR_NONE && g_Returned && !g_StatsAccepted), "preauth_stats_rejected");
 PrintToServer("[SW-IDENT] preauth observed=1 authorized=0 error=%d returned=%d", error, g_Returned);
}
public void OnClientDisconnect_Post(int client)
{
 if (client != g_FixtureSlot) return;
 g_FixtureSlot = 0; g_FixtureSerial = 0;
 Check(!IsClientConnected(client), "fixture_disconnected_post");
 ErrorCase(client, 32, SP_ERROR_NATIVE, "disconnected_identity");
 Fill(); g_Returned = false;
 Call_StartFunction(GetMyHandle(), StatsCall); Call_PushCell(client);
 int error = Call_Finish();
 if (error == SP_ERROR_NATIVE) g_ExpectedErrors++;
 Check(error == SP_ERROR_NATIVE && !g_Returned && Unchanged(), "disconnected_stats");
 PrintToServer("[SW-IDENT] native_error expected=%d observed=%d unchanged=%d returned=%d", SP_ERROR_NATIVE, error, Unchanged(), g_Returned);
 g_Disconnected++;
}
public Action CheckIdentity(int client, int args)
{
 if (client != 0) return Plugin_Handled;
 char mode[16]; GetCmdArg(1, mode, sizeof(mode));
 if (StrEqual(mode, "bot"))
 {
  int fixture = CreateFakeClient("SteamWorksIdentityFixture");
  if (fixture == 0) { Check(false, "create_fixture"); return Plugin_Handled; }
  g_FixtureSerial = GetClientSerial(fixture); g_FixtureSlot = fixture;
  Check(IsClientConnected(fixture) && IsFakeClient(fixture), "owned_fake_client_connected");
  Check(!SteamWorks_RequestStats(fixture, 550), "bot_stats_rejected");
  Buffers(fixture, false);
  ErrorCase(0, 32, SP_ERROR_NATIVE, "server_index_rejected");
  ErrorCase(MaxClients + 1, 32, SP_ERROR_NATIVE, "out_of_range_client_rejected");
  ErrorCase(0, 0, SP_ERROR_NATIVE, "invalid_client_before_zero");
  ErrorCase(0, -1, SP_ERROR_NATIVE, "invalid_client_before_negative");
  KickClient(fixture, "Identity fixture completed");
 }
 else if (StrEqual(mode, "human"))
 {
  int humans;
  for (int target = 1; target <= MaxClients; target++)
  {
   if (!IsClientInGame(target) || IsFakeClient(target) || !IsClientAuthorized(target)) continue;
   humans++; Buffers(target, true);
   Check(g_PreauthSerial[target] == GetClientSerial(target), "same_human_preauth_observed");
   Check(SteamWorks_RequestStats(target, 550), "authorized_stats_control_accepted");
  }
  PrintToServer("[SW-IDENT] humans=%d", humans);
 }
 PrintToServer("[SW-IDENT] status checks=%d failures=%d expectedErrors=%d preauth=%d disconnected=%d", g_Checks, g_Failures, g_ExpectedErrors, g_Preauth, g_Disconnected);
 return Plugin_Handled;
}
