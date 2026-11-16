#pragma semicolon 1
#pragma newdecls required
#include <sourcemod>
#include <steamworks>

public Plugin myinfo = {name = "SteamWorks Pending Owner", author = "Lechuga", version = "1.0.0"};

public void OnPluginStart()
{
	Handle request = SteamWorks_CreateHTTPRequest(k_EHTTPMethodGET, "http://127.0.0.1:27880/delay");
	if (request == null) SetFailState("Local request unavailable");
	SteamWorks_SetHTTPCallbacks(request, Completed);
	bool sent = SteamWorks_SendHTTPRequest(request);
	PrintToServer("[SW-ACCEPT-OWNER] queued=%d", sent);
	// SourceMod destroys this plugin-owned handle on unload. The extension must
	// invalidate its callback before OnPluginEnd and automatic handle cleanup.
}

public void OnPluginEnd()
{
	PrintToServer("[SW-ACCEPT-OWNER] ended");
}

public void Completed(Handle request, bool failed, bool success, EHTTPStatusCode status)
{
	PrintToServer("[SW-ACCEPT-OWNER] callback");
	delete request;
}
