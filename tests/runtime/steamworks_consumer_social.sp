#pragma semicolon 1
#pragma newdecls required
#include <sourcemod>
#include <steamworks>
#include <l4d2_familyshare_core>

public Plugin myinfo = {name="SteamWorks Consumer Social Observer", author="Lechuga", version="1.0.0"};
enum struct PluginRuntimeState
{
    bool lateLoad;
    bool hasFamily;
}
PluginRuntimeState g_Runtime;
int g_Checks, g_Failures, g_Detected, g_Group, g_GroupAccount, g_GroupEvents;
int g_ProfileRequest, g_ProfileOwner, g_ProfileEvents;

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int length)
{
    g_Runtime.lateLoad = late;
    return APLRes_Success;
}

public void OnPluginStart()
{
    g_Runtime.hasFamily = LibraryExists(L4D2_FAMILYSHARE_CORE_LIBRARY);
    RegAdminCmd("sm_sw_consumer_social", Control, ADMFLAG_ROOT);
    PrintToServer("[SW-SOCIAL] ready late=%d", g_Runtime.lateLoad);
}
public void OnAllPluginsLoaded() { g_Runtime.hasFamily = LibraryExists(L4D2_FAMILYSHARE_CORE_LIBRARY); }
public void OnLibraryAdded(const char[] name)
{
    if (StrEqual(name, L4D2_FAMILYSHARE_CORE_LIBRARY)) g_Runtime.hasFamily = true;
}
public void OnLibraryRemoved(const char[] name)
{
    if (StrEqual(name, L4D2_FAMILYSHARE_CORE_LIBRARY)) g_Runtime.hasFamily = false;
}
void Check(bool value, const char[] label)
{
    g_Checks++;
    if (!value) g_Failures++;
    PrintToServer("[SW-SOCIAL] check=%s pass=%d", label, value);
}

public Action Control(int client, int args)
{
    if (client != 0 || args < 1) return Plugin_Handled;
    char mode[16]; GetCmdArg(1, mode, sizeof(mode));
    if (StrEqual(mode, "key") && g_Runtime.hasFamily)
    {
        char path[PLATFORM_MAX_PATH], key[128];
        BuildPath(Path_SM, path, sizeof(path), "recovery/steam-api-key.txt");
        File file = OpenFile(path, "r");
        if (file == null) { PrintToServer("[SW-SOCIAL] private_key_file_missing=1"); return Plugin_Handled; }
        bool read = file.ReadLine(key, sizeof(key)); delete file;
        TrimString(key);
        ConVar convar = FindConVar("l4d2_familyshare_steam_api_key");
        Check(read && key[0] != 0 && convar != null, "private_key_input_available");
        if (read && key[0] != 0 && convar != null) convar.SetString(key, false, false);
        Check(DeleteFile(path), "private_key_input_removed");
    }
    if (StrEqual(mode, "group-config"))
    {
        char path[PLATFORM_MAX_PATH];
        BuildPath(Path_SM, path, sizeof(path), "configs/acceptance-private.cfg");
        KeyValues kv = new KeyValues("Acceptance");
        if (kv.ImportFromFile(path)) g_Group = kv.GetNum("group_accountid", 0);
        delete kv;
        PrintToServer("[SW-SOCIAL] group_configured=%d", g_Group != 0);
    }
    if (StrEqual(mode, "inspect") && g_Runtime.hasFamily)
    {
        int humans, borrowed;
        for (int i = 1; i <= MaxClients; i++)
        {
            if (!IsClientInGame(i) || IsFakeClient(i) || !IsClientAuthorized(i)) continue;
            humans++;
            if (L4D2FamilyShareCore_IsClientDetected(i)) borrowed++;
        }
        PrintToServer("[SW-SOCIAL] authorized_humans=%d borrowed_detected=%d", humans, borrowed);
    }
    if (StrEqual(mode, "profile") || StrEqual(mode, "group"))
    {
        int target;
        for (int i = 1; i <= MaxClients; i++)
            if (IsClientInGame(i) && !IsFakeClient(i) && IsClientAuthorized(i)) { target = i; break; }
        if (target == 0) { PrintToServer("[SW-SOCIAL] awaiting_authorized_human=1"); return Plugin_Handled; }
        if (StrEqual(mode, "profile") && g_Runtime.hasFamily)
        {
            bool detected = L4D2FamilyShareCore_IsClientDetected(target);
            Check(detected, "real_borrowed_license_detected");
            if (!detected) return Plugin_Handled;
            int borrower = L4D2FamilyShareCore_GetClientBorrowerAccountId(target);
            int owner = L4D2FamilyShareCore_GetClientOwnerAccountId(target);
            Check(borrower == GetSteamAccountID(target, true) && owner != 0 && owner != borrower,
                "real_consumer_owner_borrower_contract");
            bool ownerConnected;
            for (int i = 1; i <= MaxClients; i++)
                if (IsClientConnected(i) && !IsFakeClient(i) && GetSteamAccountID(i, true) == owner) ownerConnected = true;
            Check(!ownerConnected, "profile_owner_not_connected_no_persona_fallback");
            if (ownerConnected) return Plugin_Handled;
            g_ProfileOwner = owner;
            g_ProfileRequest = L4D2FamilyShareCore_RequestProfile(owner);
            Check(g_ProfileRequest > 0, "real_owner_profile_request_accepted");
        }
        if (StrEqual(mode, "group"))
        {
            if (g_Group == 0) { PrintToServer("[SW-SOCIAL] awaiting_group_config=1"); return Plugin_Handled; }
            g_GroupAccount = GetSteamAccountID(target, true);
            Check(SteamWorks_GetUserGroupStatus(target, g_Group), "real_group_request_accepted");
        }
    }
    PrintToServer("[SW-SOCIAL] status checks=%d failures=%d detected_events=%d profile_events=%d group_events=%d",
        g_Checks, g_Failures, g_Detected, g_ProfileEvents, g_GroupEvents);
    return Plugin_Handled;
}

public void L4D2FamilyShareCore_OnDetected(int client, int borrower, int owner)
{
    if (client < 1 || client > MaxClients || !IsClientConnected(client)) return;
    Check(borrower != 0 && owner != 0 && borrower != owner, "real_family_detection_forward_distinct_accounts");
    g_Detected++;
    PrintToServer("[SW-SOCIAL] real_detection_event=1");
}
public void L4D2FamilyShareCore_OnProfileResolved(int requestId, int accountId, const char[] steamId64, const char[] persona, const char[] url)
{
    if (requestId != g_ProfileRequest || g_ProfileRequest == 0) return;
    g_ProfileEvents++;
    char expected[256]; FormatEx(expected, sizeof(expected), "https://steamcommunity.com/profiles/%s", steamId64);
    Check(accountId == g_ProfileOwner && strlen(steamId64) == 17 && StrEqual(url, expected), "real_profile_identity_transport");
    // The requested owner is absent: nonempty persona cannot be the client-name fallback.
    Check(persona[0] != 0, "real_Web_API_profile_persona_resolved");
    Check(g_ProfileEvents == 1, "one_profile_completion");
    g_ProfileRequest = 0;
    PrintToServer("[SW-SOCIAL] profile_complete checks=%d failures=%d", g_Checks, g_Failures);
}
public void SteamWorks_OnClientGroupStatus(int accountId, int group, bool member, bool officer)
{
    if (accountId != g_GroupAccount || group != g_Group || g_Group == 0) return;
    g_GroupEvents++;
    Check(member, "authorized_test_group_membership_positive");
    PrintToServer("[SW-SOCIAL] group_complete member=%d officer=%d checks=%d failures=%d", member, officer, g_Checks, g_Failures);
}
