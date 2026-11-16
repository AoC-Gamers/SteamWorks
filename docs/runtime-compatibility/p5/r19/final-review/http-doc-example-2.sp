#pragma semicolon 1
#pragma newdecls required
#include <sourcemod>
#include <steamworks>
public void OnPluginStart() {}
public void DocumentationExample(Handle request) {
int size;
char body[1024];
if (SteamWorks_GetHTTPResponseBodySize(request, size) && size >= 0 && size < sizeof(body))
{
	if (size == 0 || SteamWorks_GetHTTPResponseBodyData(request, body, size))
	{
		body[size] = '\0';
		PrintToServer("body=%s", body);
	}
}

}
