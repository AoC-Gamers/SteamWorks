#pragma semicolon 1
#pragma newdecls required
#include <sourcemod>
#include <steamworks>
public void OnPluginStart() {}
public void DocumentationExample(Handle request) {
char body[2048];
int written;
bool truncated;
if (SteamWorks_GetHTTPResponseBodyString(request, body, sizeof(body), written, truncated))
{
	PrintToServer("written=%d truncated=%d body=%s", written, truncated, body);
}

}
