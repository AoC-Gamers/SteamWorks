#pragma semicolon 1
#pragma newdecls required
#include <sourcemod>
#include <steamworks>

public Plugin myinfo =
{
 name = "SteamWorks Lifecycle Pending Request",
 author = "Lechuga",
 description = "Hold one request across a real runtime shutdown in the isolated lab",
 version = "1.0.0"
};

Handle g_Request;
int g_Callbacks;
int g_MapEnds;

public void OnPluginStart()
{
 RegAdminCmd("sm_sw_lifecycle_pending", CheckPending, ADMFLAG_ROOT);
 PrintToServer("[SW-PENDING] plugin_ready");
}

// Deliberately keep the plugin-owned handle across the map boundary: this
// fixture tests runtime invalidation, not OnMapEnd cancellation by the plugin.
public void OnMapEnd() { g_MapEnds++; }
public void OnPluginEnd() { delete g_Request; }

public Action CheckPending(int client, int args)
{
 char command[16];
 GetCmdArg(1, command, sizeof(command));
 if (StrEqual(command, "queue"))
 {
  if (g_Request != null) { PrintToServer("[SW-PENDING] busy"); return Plugin_Handled; }
  g_Request = SteamWorks_CreateHTTPRequest(k_EHTTPMethodGET, "http://127.0.0.1:27880/hold");
  if (g_Request == null) { PrintToServer("[SW-PENDING] create_failed"); return Plugin_Handled; }
  bool configured = SteamWorks_SetHTTPRequestContextValue(g_Request, 31, 0x12345678)
   && SteamWorks_SetHTTPRequestAbsoluteTimeoutMS(g_Request, 60000)
   && SteamWorks_SetHTTPCallbacks(g_Request, Completed);
  bool sent = configured && SteamWorks_SendHTTPRequest(g_Request);
  PrintToServer("[SW-PENDING] queued configured=%d sent=%d", configured, sent);
 }
 else if (StrEqual(command, "check"))
 {
  // An obsolete request correctly rejects native operations with an exception.
  // Keep that check intact; observe cancellation through callback silence and
  // subsequent safe handle close after the fixture releases its response.
  PrintToServer("[SW-PENDING] retained pass=%d owned=%d callbacks=%d mapEnds=%d",
   g_Request != null && g_Callbacks == 0, g_Request != null, g_Callbacks, g_MapEnds);
 }
 else if (StrEqual(command, "close"))
 {
  delete g_Request;
  PrintToServer("[SW-PENDING] closed callbacks=%d mapEnds=%d", g_Callbacks, g_MapEnds);
 }
 return Plugin_Handled;
}

public void Completed(Handle request, bool failure, bool success, EHTTPStatusCode status, any high, any low)
{
 g_Callbacks++;
 PrintToServer("[SW-PENDING] FAIL stale_callback");
}
