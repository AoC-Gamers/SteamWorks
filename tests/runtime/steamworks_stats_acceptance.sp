#pragma semicolon 1
#pragma newdecls required
#include <sourcemod>
#include <steamworks>
#include "stats_schema_keys.inc"

public Plugin myinfo =
{
 name = "SteamWorks Stats Acceptance",
 author = "Lechuga",
 description = "Read-only AppID 550 float and missing-key acceptance",
 version = "1.0.0"
};

Handle g_Timer;
int g_UserId, g_Account, g_Index, g_Floats, g_Failures;
float g_Deadline, g_ReadyAfter;
bool g_Control;

public void OnPluginStart()
{
 RegAdminCmd("sm_sw_stats_accept", StartAcceptance, ADMFLAG_ROOT);
 PrintToServer("[SW-STATS] plugin_ready schema_keys=%d", sizeof(g_StatKeys));
}

void StopAcceptance()
{
 delete g_Timer;
 g_UserId = 0;
 g_Account = 0;
}

public void OnMapEnd() { StopAcceptance(); }
public void OnPluginEnd() { StopAcceptance(); }
public void OnClientDisconnect(int client)
{
 if (g_Timer != null && GetClientUserId(client) == g_UserId)
 {
  PrintToServer("[SW-STATS] aborted reason=client_disconnect");
  StopAcceptance();
 }
}

public Action StartAcceptance(int client, int args)
{
 if (g_Timer != null) { PrintToServer("[SW-STATS] busy"); return Plugin_Handled; }
 int target, humans;
 for (int i = 1; i <= MaxClients; i++)
 {
  if (!IsClientInGame(i) || IsFakeClient(i) || !IsClientAuthorized(i)) continue;
  target = i; humans++;
 }
 if (humans != 1) { PrintToServer("[SW-STATS] waiting authorized_humans=%d", humans); return Plugin_Handled; }
 g_UserId = GetClientUserId(target);
 g_Account = GetSteamAccountID(target, true);
 bool byClient = SteamWorks_RequestStats(target, 550);
 bool byAccount = SteamWorks_RequestStatsAuthID(g_Account, 550);
 PrintToServer("[SW-STATS] requested client=%d account=%d", byClient, byAccount);
 if (!byClient || !byAccount || g_Account == 0) { g_Account = 0; return Plugin_Handled; }
 g_Index = 0; g_Floats = 0; g_Failures = 0; g_Control = false;
 g_ReadyAfter = GetGameTime() + 3.0;
 g_Deadline = GetGameTime() + 20.0;
 g_Timer = CreateTimer(0.1, PollStats, _, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
 return Plugin_Handled;
}

bool MissingKey(int target)
{
 int c = 0x12345678, a = 0x76543210;
 float cf = view_as<float>(0x7fc01001), af = view_as<float>(0x7fc02002);
 bool ci = SteamWorks_GetStatCell(target, MISSING_STAT, c);
 bool ai = SteamWorks_GetStatAuthIDCell(g_Account, MISSING_STAT, a);
 bool cr = SteamWorks_GetStatFloat(target, MISSING_STAT, cf);
 bool ar = SteamWorks_GetStatAuthIDFloat(g_Account, MISSING_STAT, af);
 bool ints = c == 0x12345678 && a == 0x76543210;
 bool floats = view_as<int>(cf) == 0x7fc01001 && view_as<int>(af) == 0x7fc02002;
 bool ok = !ci && !ai && !cr && !ar && ints && floats;
 PrintToServer("[SW-STATS] missing pass=%d clientInt=%d accountInt=%d clientFloat=%d accountFloat=%d intPreserved=%d floatBitsPreserved=%d",
  ok, ci, ai, cr, ar, ints, floats);
 return ok;
}

public Action PollStats(Handle timer)
{
 int target = GetClientOfUserId(g_UserId);
 if (target == 0 || !IsClientInGame(target) || IsFakeClient(target) || !IsClientAuthorized(target)
  || GetSteamAccountID(target, true) != g_Account)
 {
  PrintToServer("[SW-STATS] aborted reason=identity_changed");
  g_Timer = null; return Plugin_Stop;
 }
 if (GetGameTime() < g_ReadyAfter) return Plugin_Continue;
 if (!g_Control)
 {
  int c = 0x12345678, a = 0x76543210;
  bool ci = SteamWorks_GetStatCell(target, "Stat.GamesPlayed.Total", c);
  bool ai = SteamWorks_GetStatAuthIDCell(g_Account, "Stat.GamesPlayed.Total", a);
  if (!ci || !ai || c != a)
  {
   if (GetGameTime() < g_Deadline) return Plugin_Continue;
   PrintToServer("[SW-STATS] aborted reason=integer_control_not_ready");
   g_Timer = null; return Plugin_Stop;
  }
  g_Control = true;
  PrintToServer("[SW-STATS] integer_control pass=1");
  if (!MissingKey(target)) g_Failures++;
 }
 int end = g_Index + 16;
 if (end > sizeof(g_StatKeys)) end = sizeof(g_StatKeys);
 for (; g_Index < end; g_Index++)
 {
  float c = view_as<float>(0x7fc01001), a = view_as<float>(0x7fc02002);
  bool ci = SteamWorks_GetStatFloat(target, g_StatKeys[g_Index], c);
  bool ai = SteamWorks_GetStatAuthIDFloat(g_Account, g_StatKeys[g_Index], a);
  if (!ci && !ai)
  {
   if (view_as<int>(c) != 0x7fc01001 || view_as<int>(a) != 0x7fc02002) g_Failures++;
   continue;
  }
  // IEEE-754 exponent all ones means infinity/NaN, including our sentinels.
  bool finite = (view_as<int>(c) & 0x7f800000) != 0x7f800000
   && (view_as<int>(a) & 0x7f800000) != 0x7f800000;
  bool ok = ci && ai && finite && view_as<int>(c) == view_as<int>(a);
  if (ok) g_Floats++;
  else g_Failures++;
  PrintToServer("[SW-STATS] float key=%s client=%d account=%d finite=%d matches=%d pass=%d",
   g_StatKeys[g_Index], ci, ai, finite, view_as<int>(c) == view_as<int>(a), ok);
 }
 if (g_Index < sizeof(g_StatKeys)) return Plugin_Continue;
 bool missing = MissingKey(target);
 if (!missing) g_Failures++;
 PrintToServer("[SW-STATS] complete schema_keys=%d float_keys=%d failures=%d pass=%d",
  g_Index, g_Floats, g_Failures, g_Floats > 0 && g_Failures == 0);
 g_Timer = null;
 return Plugin_Stop;
}
