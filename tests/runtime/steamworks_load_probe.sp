#pragma semicolon 1
#pragma newdecls required
#include <sourcemod>
// Deliberately no SharedExtension descriptor: probe the native owner loaded by
// either name. A separate consumer with steamworks.inc tests logical resolution.
native bool SteamWorks_IsLoaded();
native bool SteamWorks_IsConnected();
native Handle SteamWorks_CreateHTTPRequest(int method, const char[] url);
native bool SteamWorks_SetHTTPRequestContextValue(Handle request, any high, any low);
native bool SteamWorks_SetHTTPCallbacks(Handle request, Function completed,
    Function headers = INVALID_FUNCTION, Function data = INVALID_FUNCTION, Handle caller = null);
native bool SteamWorks_SendHTTPRequest(Handle request);
native bool SteamWorks_GetHTTPResponseBodySize(Handle request, int &size);
native bool SteamWorks_GetHTTPResponseBodyData(Handle request, char[] body, int length);

public Plugin myinfo = {name = "SteamWorks Load Ownership Probe", author = "AoC-Gamers", version = "1.0.0"};
Handle g_Request;
ConVar g_Debug;
int g_Completed, g_Failed;
public void OnPluginStart()
{
    g_Debug = FindConVar("steamworks_debug");
    RegServerCmd("sw_load_probe", Probe);
    PrintToServer("[load-probe] ready=1 debug=%d", g_Debug != null);
}
public void OnPluginEnd() { delete g_Request; }
public Action Probe(int args)
{
    char mode[16]; GetCmdArg(1, mode, sizeof(mode));
    bool same = g_Debug != null && FindConVar("steamworks_debug") == g_Debug;
    PrintToServer("[load-probe] loaded=%d connected=%d same_debug=%d completed=%d failed=%d pending=%d",
        SteamWorks_IsLoaded(), SteamWorks_IsConnected(), same, g_Completed, g_Failed, g_Request != null);
    if (!StrEqual(mode, "hold") || g_Request != null) return Plugin_Handled;
    g_Request = SteamWorks_CreateHTTPRequest(1, "http://127.0.0.1:27880/delay");
    bool context = g_Request != null && SteamWorks_SetHTTPRequestContextValue(g_Request, 31, 0x12345678);
    bool callbacks = context && SteamWorks_SetHTTPCallbacks(g_Request, Completed);
    bool sent = callbacks && SteamWorks_SendHTTPRequest(g_Request);
    if (!sent) { g_Failed++; delete g_Request; }
    PrintToServer("[load-probe] queued context=%d callbacks=%d sent=%d", context, callbacks, sent);
    return Plugin_Handled;
}
public void Completed(Handle request, bool failure, bool success, int status, any high, any low)
{
    int size = -1;
    char body[8];
    bool read = SteamWorks_GetHTTPResponseBodySize(request, size) && size == 7
        && SteamWorks_GetHTTPResponseBodyData(request, body, 7);
    body[7] = 0;
    bool context = high == 31 && low == 0x12345678;
    bool ok = request == g_Request && !failure && success && status == 200
        && read && StrEqual(body, "delayed") && context;
    g_Completed++;
    if (!ok) g_Failed++;
    PrintToServer("[load-probe] complete pass=%d context=%d body=%d count=%d failures=%d",
        ok, context, read && StrEqual(body, "delayed"), g_Completed, g_Failed);
    delete g_Request;
}
