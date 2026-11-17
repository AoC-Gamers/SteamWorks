#pragma semicolon 1
#pragma newdecls required
#include <sourcemod>
#include <steamworks>

public Plugin myinfo = {name="SteamWorks Pending Identity Observer", author="Lechuga", version="1.0.0"};
bool g_Armed, g_Returned, g_Accepted;
int g_Checks, g_Failures, g_PendingObserved;
int g_PendingSerial[MAXPLAYERS + 1];

void Check(bool value, const char[] label)
{
    g_Checks++;
    if (!value) g_Failures++;
    PrintToServer("[SW-PENDING] check=%s pass=%d", label, value);
}

public void OnPluginStart()
{
    RegAdminCmd("sm_sw_identity_pending", Control, ADMFLAG_ROOT);
    PrintToServer("[SW-PENDING] ready");
}

public Action Control(int client, int args)
{
    if (client != 0) return Plugin_Handled;
    char mode[16]; GetCmdArg(1, mode, sizeof(mode));
    // The host verifies the loaded provider SHA and audited 12-bit entry layout.
    if (StrEqual(mode, "arm")) g_Armed = true;
    if (StrEqual(mode, "authorized"))
    {
        int humans;
        for (int i = 1; i <= MaxClients; i++)
        {
            if (!IsClientInGame(i) || IsFakeClient(i) || !IsClientAuthorized(i)) continue;
            humans++;
            Check(g_PendingSerial[i] == GetClientSerial(i), "same_real_client_after_authorization");
            Check(SteamWorks_RequestStats(i, 550), "authorized_positive_request_control");
        }
        Check(humans > 0, "authorized_human_present");
    }
    PrintToServer("[SW-PENDING] status armed=%d observations=%d checks=%d failures=%d",
        g_Armed, g_PendingObserved, g_Checks, g_Failures);
    return Plugin_Handled;
}

public void RequestStats(int reference)
{
    g_Accepted = SteamWorks_RequestStats(reference, 550);
    g_Returned = true;
}

public void OnClientConnected(int client)
{
    if (!g_Armed || !IsClientConnected(client) || IsFakeClient(client) || IsClientAuthorized(client)) return;
    int reference = INVALID_ENT_REFERENCE;
    // Read-only lookup using the provider's public conversion. No player/auth
    // fields or entity serials are modified, and no invalid native calls/logs.
    for (int serial; serial < 4096; serial++)
    {
        int candidate = view_as<int>(0x80000000) | (serial << 12) | client;
        if (EntRefToEntIndex(candidate) == client) { reference = candidate; break; }
    }
    Check(reference != INVALID_ENT_REFERENCE, "current_pending_slot_serial_reference_found");
    if (reference == INVALID_ENT_REFERENCE) return;
    Check(EntRefToEntIndex(reference) == client && IsClientConnected(client)
        && !IsFakeClient(client) && !IsClientAuthorized(client), "real_connected_nonbot_unauthorized_client");
    g_Returned = false; g_Accepted = true;
    Call_StartFunction(GetMyHandle(), RequestStats); Call_PushCell(reference);
    int error = Call_Finish();
    Check(error == SP_ERROR_NONE && g_Returned && !g_Accepted, "unauthorized_native_returns_false_without_exception");
    g_PendingObserved++;
    g_PendingSerial[client] = GetClientSerial(client);
    PrintToServer("[SW-PENDING] pending observed=1 native_error=%d returned=%d accepted=%d",
        error, g_Returned, g_Accepted);
}
