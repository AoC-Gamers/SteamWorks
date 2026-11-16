#pragma newdecls required
#include <sourcemod>
#include <steamworks>
public void ThreeArgumentCall(Handle request) {char body[64];SteamWorks_GetHTTPResponseBodyString(request,body,sizeof(body));}
