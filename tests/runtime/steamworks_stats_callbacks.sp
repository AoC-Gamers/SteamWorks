#pragma semicolon 1
#pragma newdecls required
#include <sourcemod>
#include <steamworks>

public Plugin myinfo =
{
    name = "SteamWorks Stats Callbacks",
    author = "AoC-Gamers",
    description = "Read-only asynchronous stats acceptance without identity logging",
    version = "1.0.0"
};

int g_Received, g_Failed, g_Unloaded;
int g_Account, g_UserId, g_Phase, g_Before, g_Failures;
// Independent observation survives completion, disconnects and map changes.
int g_ObservedAccount, g_TargetUnloaded;
Handle g_Timer;

public void OnPluginStart()
{
    RegServerCmd("sw_stats_callbacks_status", Status);
    RegServerCmd("sw_stats_callbacks_test", StartTest);
    RegServerCmd("sw_stats_callbacks_reset", ResetObservation);
    PrintToServer("[stats-callbacks] ready=1");
}

void ResetTest()
{
    delete g_Timer;
    g_Account = 0;
    g_UserId = 0;
    g_Phase = 0;
}

public void OnMapEnd() { ResetTest(); }
public void OnClientDisconnect(int client)
{
    if (g_Phase != 0 && GetClientUserId(client) == g_UserId)
    {
        PrintToServer("[stats-callbacks] aborted=client_disconnect");
        ResetTest();
    }
}

public Action StartTest(int args)
{
    if (g_Phase != 0)
    {
        PrintToServer("[stats-callbacks] busy=1");
        return Plugin_Handled;
    }
    int target, humans;
    for (int client = 1; client <= MaxClients; client++)
    {
        if (!IsClientInGame(client) || IsFakeClient(client) || !IsClientAuthorized(client))
            continue;
        target = client;
        humans++;
    }
    if (humans != 1)
    {
        PrintToServer("[stats-callbacks] waiting authorized_humans=%d", humans);
        return Plugin_Handled;
    }
    g_UserId = GetClientUserId(target);
    g_Account = GetSteamAccountID(target, true);
    g_Before = g_Received;
    g_Failures = 0;
    g_Phase = 1;
    bool clientRequest = SteamWorks_RequestStats(target, 550);
    bool accountRequest = SteamWorks_RequestStatsAuthID(g_Account, 550);
    bool duplicate = SteamWorks_RequestStatsAuthID(g_Account, 550);
    bool accepted = g_Account != 0 && clientRequest && accountRequest && duplicate;
    PrintToServer("[stats-callbacks] requested phase=1 client=%d account=%d duplicate=%d pass=%d",
        clientRequest, accountRequest, duplicate, accepted);
    if (!accepted) { ResetTest(); return Plugin_Handled; }
    g_ObservedAccount = g_Account;
    g_TargetUnloaded = 0;
    g_Timer = CreateTimer(20.0, TimedOut, _, TIMER_FLAG_NO_MAPCHANGE);
    return Plugin_Handled;
}

public Action TimedOut(Handle timer)
{
    g_Timer = null;
    PrintToServer("[stats-callbacks] timeout=1 phase=%d callbacks=%d pass=0", g_Phase, g_Received - g_Before);
    ResetTest();
    return Plugin_Stop;
}

public void SteamWorks_OnUserStatsReceived(int authid, EResult result, int appid)
{
    g_Received++;
    if (result != k_EResultOK) g_Failed++;
    if (g_Phase == 0) return;
    bool identity = authid == g_Account;
    bool app = appid == 550;
    bool ok = identity && app && result == k_EResultOK;
    if (!ok) g_Failures++;
    PrintToServer("[stats-callbacks] received phase=%d result=%d identity_match=%d app_match=%d pass=%d",
        g_Phase, result, identity, app, ok);
    if (g_Phase != 1 && g_Phase != 2) { g_Failures++; return; }
    int client = GetClientOfUserId(g_UserId);
    if (client == 0 || !IsClientInGame(client) || !IsClientAuthorized(client)
        || GetSteamAccountID(client, true) != g_Account)
    {
        PrintToServer("[stats-callbacks] aborted=identity_changed");
        ResetTest();
        return;
    }
    int byClient = 0x12345678, byAccount = 0x76543210;
    bool clientRead = false, accountRead = false;
    if (ok)
    {
        clientRead = SteamWorks_GetStatCell(client, "Stat.GamesPlayed.Total", byClient);
        accountRead = SteamWorks_GetStatAuthIDCell(g_Account, "Stat.GamesPlayed.Total", byAccount);
    }
    bool reads = clientRead && accountRead && byClient == byAccount;
    if (!reads) g_Failures++;
    PrintToServer("[stats-callbacks] reads_after_result client=%d account=%d equal=%d pass=%d",
        clientRead, accountRead, byClient == byAccount, reads);
    delete g_Timer;
    if (g_Phase == 1)
    {
        g_Phase = 3;
        g_Timer = CreateTimer(1.0, RequestAgain, _, TIMER_FLAG_NO_MAPCHANGE);
    }
    else
    {
        g_Phase = 4;
        g_Timer = CreateTimer(1.0, FinishTest, _, TIMER_FLAG_NO_MAPCHANGE);
    }
}

public Action RequestAgain(Handle timer)
{
    g_Timer = null;
    if (g_Received - g_Before != 1) g_Failures++;
    g_Phase = 2;
    bool accepted = SteamWorks_RequestStatsAuthID(g_Account, 550);
    PrintToServer("[stats-callbacks] requested phase=2 account=%d", accepted);
    if (!accepted) { PrintToServer("[stats-callbacks] rejected=1 pass=0"); ResetTest(); return Plugin_Stop; }
    g_Timer = CreateTimer(20.0, TimedOut, _, TIMER_FLAG_NO_MAPCHANGE);
    return Plugin_Stop;
}

public Action FinishTest(Handle timer)
{
    g_Timer = null;
    bool ok = g_Received - g_Before == 2 && g_Failures == 0;
    PrintToServer("[stats-callbacks] complete callbacks=%d failures=%d pass=%d",
        g_Received - g_Before, g_Failures, ok);
    ResetTest();
    return Plugin_Stop;
}

public void SteamWorks_OnUserStatsUnloaded(int authid)
{
    g_Unloaded++;
    bool matches = g_ObservedAccount != 0 && authid == g_ObservedAccount;
    if (matches) g_TargetUnloaded++;
    if (g_Unloaded <= 20)
        PrintToServer("[stats-callbacks] cache_unloaded=1 target_match=%d", matches);
}

public Action ResetObservation(int args)
{
    g_ObservedAccount = 0;
    g_TargetUnloaded = 0;
    PrintToServer("[stats-callbacks] observation_reset=1");
    return Plugin_Handled;
}

public Action Status(int args)
{
    PrintToServer("[stats-callbacks] received=%d failed=%d unloaded=%d phase=%d watching=%d matched_unloaded=%d",
        g_Received, g_Failed, g_Unloaded, g_Phase, g_ObservedAccount != 0, g_TargetUnloaded);
    return Plugin_Handled;
}
