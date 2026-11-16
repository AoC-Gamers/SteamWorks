#pragma semicolon 1
#pragma newdecls required
#include <sourcemod>
#include <steamworks>

public Plugin myinfo =
{
 name = "SteamWorks Rollback Acceptance",
 author = "Lechuga",
 description = "Baseline-compatible read-only runtime smoke for archived rollback",
 version = "1.0.0"
};

Handle g_Request, g_CloseTimer;
int g_Completed, g_Failed;

public void OnPluginStart()
{
 RegAdminCmd("sm_sw_restore_check", CheckRestore, ADMFLAG_ROOT);
 PrintToServer("[SW-RESTORE] plugin_ready");
}

void CloseRequest()
{
 delete g_CloseTimer;
 delete g_Request;
}

public void OnMapEnd() { CloseRequest(); }
public void OnPluginEnd()
{
 CloseRequest();
 PrintToServer("[SW-RESTORE] plugin_end completed=%d failed=%d", g_Completed, g_Failed);
}

public Action CheckRestore(int client, int args)
{
 PrintToServer("[SW-RESTORE] status loaded=%d connected=%d vac=%d completed=%d failed=%d",
  SteamWorks_IsLoaded(), SteamWorks_IsConnected(), SteamWorks_IsVACEnabled(), g_Completed, g_Failed);
 char mode[16]; GetCmdArg(1, mode, sizeof(mode));
 if (!StrEqual(mode, "http")) return Plugin_Handled;
 if (g_Request != null) { PrintToServer("[SW-RESTORE] busy"); return Plugin_Handled; }
 g_Request = SteamWorks_CreateHTTPRequest(k_EHTTPMethodGET, "http://127.0.0.1:27880/text");
 if (g_Request == null) { g_Failed++; PrintToServer("[SW-RESTORE] create_failed"); return Plugin_Handled; }
 bool context = SteamWorks_SetHTTPRequestContextValue(g_Request, 21, 0x12345678);
 bool callbacks = SteamWorks_SetHTTPCallbacks(g_Request, Completed);
 bool sent = context && callbacks && SteamWorks_SendHTTPRequest(g_Request);
 if (!sent) { g_Failed++; CloseRequest(); }
 PrintToServer("[SW-RESTORE] queued context=%d callbacks=%d sent=%d", context, callbacks, sent);
 return Plugin_Handled;
}

public void Completed(Handle request, bool failed, bool success, EHTTPStatusCode status, any high, any low)
{
 g_Completed++;
 int size = -123;
 char body[17];
 bool sizeOk = SteamWorks_GetHTTPResponseBodySize(request, size);
 bool read = sizeOk && size == 16 && SteamWorks_GetHTTPResponseBodyData(request, body, 16);
 body[16] = 0;
 bool context = high == 21 && low == 0x12345678;
 bool ok = request == g_Request && !failed && success && view_as<int>(status) == 200
  && read && StrEqual(body, "steamworks-local") && context;
 if (!ok) g_Failed++;
 PrintToServer("[SW-RESTORE] complete pass=%d failure=%d success=%d status=%d body=%d context=%d",
  ok, failed, success, status, read && StrEqual(body, "steamworks-local"), context);
 // Avoid depending on the candidate's safe-close-in-forward improvement in the old baseline.
 g_CloseTimer = CreateTimer(0.2, DeferredClose, _, TIMER_FLAG_NO_MAPCHANGE);
}

public Action DeferredClose(Handle timer)
{
 g_CloseTimer = null;
 delete g_Request;
 PrintToServer("[SW-RESTORE] request_closed");
 return Plugin_Stop;
}
