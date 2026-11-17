#pragma semicolon 1
#pragma newdecls required
#include <sourcemod>
#include <steamworks>

public Plugin myinfo = {name = "SteamWorks Pending Owner", author = "Lechuga", version = "1.0.0"};

Handle g_Request;
int g_Sequence, g_Callbacks;

public void OnPluginStart()
{
	RegAdminCmd("sm_sw_owner", CheckOwner, ADMFLAG_ROOT);
	PrintToServer("[SW-OWNER] ready");
}

public void OnPluginEnd()
{
	// Deliberate fixture: exercise SourceMod's automatic owner-handle cleanup,
	// without explicit delete hiding the extension's OnPluginWillUnload path.
	PrintToServer("[SW-OWNER] ended pending=%d callbacks=%d sequence=%d", g_Request != null, g_Callbacks, g_Sequence);
}

public Action CheckOwner(int client, int args)
{
	char mode[16], argument[16];
	GetCmdArg(1, mode, sizeof(mode));
	GetCmdArg(2, argument, sizeof(argument));
	if (StrEqual(mode, "status"))
	{
		PrintToServer("[SW-OWNER] status pending=%d callbacks=%d sequence=%d", g_Request != null, g_Callbacks, g_Sequence);
		return Plugin_Handled;
	}
	if (g_Request != null || (!StrEqual(mode, "hold") && !StrEqual(mode, "probe")))
	{
		PrintToServer("[SW-OWNER] FAIL busy_or_invalid_mode");
		return Plugin_Handled;
	}
	g_Sequence = StringToInt(argument);
	char url[128];
	if (StrEqual(mode, "hold")) Format(url, sizeof(url), "http://127.0.0.1:27880/hold/%d", g_Sequence);
	else Format(url, sizeof(url), "http://127.0.0.1:27880/text/%d", g_Sequence);
	g_Request = SteamWorks_CreateHTTPRequest(k_EHTTPMethodGET, url);
	if (g_Request == null) SetFailState("Local request unavailable");
	bool configured = SteamWorks_SetHTTPRequestContextValue(g_Request, g_Sequence, 0x12345678)
		&& SteamWorks_SetHTTPRequestAbsoluteTimeoutMS(g_Request, 60000)
		&& SteamWorks_SetHTTPCallbacks(g_Request, Completed);
	bool sent = configured && SteamWorks_SendHTTPRequest(g_Request);
	if (!sent) delete g_Request;
	PrintToServer("[SW-OWNER] queued sequence=%d hold=%d configured=%d sent=%d", g_Sequence, StrEqual(mode, "hold"), configured, sent);
	return Plugin_Handled;
}

public void Completed(Handle request, bool failed, bool success, EHTTPStatusCode status, any sequence, any marker)
{
	g_Callbacks++;
	int size = -1;
	char body[17];
	bool read = SteamWorks_GetHTTPResponseBodySize(request, size) && size == 16
		&& SteamWorks_GetHTTPResponseBodyData(request, body, 16);
	body[16] = 0;
	bool ok = request == g_Request && !failed && success && view_as<int>(status) == 200
		&& sequence == g_Sequence && marker == 0x12345678 && read && StrEqual(body, "steamworks-local");
	PrintToServer("[SW-OWNER] callback sequence=%d context=%d pass=%d", sequence, marker == 0x12345678, ok);
	delete request;
	g_Request = null;
}
