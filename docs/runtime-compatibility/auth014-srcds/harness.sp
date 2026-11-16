#pragma semicolon 1
#pragma newdecls required
#include <sourcemod>
#include <steamworks>

public Plugin myinfo =
{
	name = "SteamWorks Acceptance",
	author = "Lechuga",
	description = "Isolated runtime contract checks with local HTTP fixtures",
	version = "1.0.0"
};

int g_Completed;
int g_Failed;
int g_Headers;
int g_StreamBytes;
int g_AuthEvents;
int g_ValidationEvents;
int g_LegacyValidationEvents;
bool g_Cancelled;

public void OnPluginStart()
{
	RegAdminCmd("sm_sw_accept", Command_Accept, ADMFLAG_ROOT);
	ConVar debug = FindConVar("steamworks_debug");
	if (debug == null) SetFailState("Extension debug convar unavailable");
	debug.IntValue = 12;
	PrintToServer("[SW-ACCEPT] debug_mask=%d", debug.IntValue);
	PrintToServer("[SW-ACCEPT] plugin_ready");
}

public void OnMapStart()
{
	PrintToServer("[SW-ACCEPT] map_start loaded=%d connected=%d", SteamWorks_IsLoaded(), SteamWorks_IsConnected());
}

public void OnPluginEnd()
{
	PrintToServer("[SW-ACCEPT] plugin_end completed=%d failed=%d", g_Completed, g_Failed);
}

public void SteamWorks_BeginAuthSession(const char[] ticket, int length, int authid)
{
	// Count only. Never inspect, print or persist an actual authentication ticket.
	g_AuthEvents++;
}

public void SteamWorks_OnValidateClient(int owner, int client)
{
	g_ValidationEvents++;
}

public void SW_OnValidateClient(int owner, int client)
{
	g_LegacyValidationEvents++;
}

public void SteamWorks_SteamServersConnected()
{
	PrintToServer("[SW-ACCEPT] connected_event");
}

public void SteamWorks_SteamServersDisconnected(EResult result)
{
	PrintToServer("[SW-ACCEPT] disconnected_event result=%d", result);
}

public Action Command_Accept(int client, int args)
{
	char mode[24];
	GetCmdArg(1, mode, sizeof(mode));
	if (StrEqual(mode, "http"))
	{
		for (int test = 0; test < 8; test++) QueueRequest(test);
	}
	else if (StrEqual(mode, "probe"))
	{
		QueueRequest(8);
	}
	else if (StrEqual(mode, "repeat"))
	{
		for (int i = 0; i < 20; i++) QueueRequest(0);
	}
	else if (StrEqual(mode, "cancel"))
	{
		g_Cancelled = true;
		Handle request = SteamWorks_CreateHTTPRequest(k_EHTTPMethodGET, "http://127.0.0.1:27880/delay");
		if (request != null)
		{
			SteamWorks_SetHTTPRequestContextValue(request, 99, 0x12345678);
			SteamWorks_SetHTTPCallbacks(request, Completed);
			bool sent = SteamWorks_SendHTTPRequest(request);
			delete request;
			PrintToServer("[SW-ACCEPT] cancel_sent=%d", sent);
		}
	}
	else if (StrEqual(mode, "stats"))
	{
		int integer = 0x12345678;
		float real = 1234.5;
		bool cellOk = SteamWorks_GetStatAuthIDCell(123456789, "__steamworks_missing_stat__", integer);
		bool floatOk = SteamWorks_GetStatAuthIDFloat(123456789, "__steamworks_missing_stat__", real);
		bool wrongApp = SteamWorks_RequestStatsAuthID(123456789, 1);
		bool invalidId = SteamWorks_RequestStatsAuthID(0, 550);
		PrintToServer("[SW-ACCEPT] stats_failure pass=%d cell=%d float=%d wrongApp=%d invalidId=%d",
			!cellOk && !floatOk && integer == 0x12345678 && real == 1234.5 && !wrongApp && !invalidId,
			cellOk, floatOk, wrongApp, invalidId);
	}
	else if (StrEqual(mode, "clients"))
	{
		int humans;
		for (int target = 1; target <= MaxClients; target++)
		{
			if (!IsClientInGame(target) || IsFakeClient(target) || !IsClientAuthorized(target)) continue;
			humans++;
			char identity[32], validated[32], small[4];
			int count = SteamWorks_GetClientSteamID(target, identity, sizeof(identity));
			bool authorized = GetClientAuthId(target, AuthId_SteamID64, validated, sizeof(validated), true);
			int smallCount = SteamWorks_GetClientSteamID(target, small, sizeof(small));
			char empty[4] = "abc";
			int emptyCount = SteamWorks_GetClientSteamID(target, empty, 0);
			bool identityOk = authorized && StrEqual(identity, validated) && count == strlen(identity) + 1
				&& smallCount == sizeof(small) && small[3] == 0 && emptyCount == 0 && StrEqual(empty, "abc");
			bool clientStats = SteamWorks_RequestStats(target, 550);
			int account = GetSteamAccountID(target, true);
			bool authStats = SteamWorks_RequestStatsAuthID(account, 550);
			PrintToServer("[SW-ACCEPT] client_identity pass=%d clientStats=%d authStats=%d license=%d", identityOk, clientStats, authStats, SteamWorks_HasLicenseForApp(target, 550));
			CreateTimer(3.0, ReadStats, account);
		}
		PrintToServer("[SW-ACCEPT] authorized_humans=%d", humans);
	}
	else if (StrEqual(mode, "authstats"))
	{
		char argument[24];
		GetCmdArg(2, argument, sizeof(argument));
		int account = StringToInt(argument);
		PrintToServer("[SW-ACCEPT] authorized_stats_request accepted=%d", SteamWorks_RequestStatsAuthID(account, 550));
		CreateTimer(3.0, ReadStats, account);
	}
	else if (StrEqual(mode, "metadata"))
	{
		SteamWorks_SetGameDescription("SteamWorks isolated acceptance");
		SteamWorks_SetGameData("isolated acceptance");
		SteamWorks_SetMapName("c1m1_hotel");
		SteamWorks_SetRule("sw_acceptance", "isolated");
		SteamWorks_ClearRules();
		PrintToServer("[SW-ACCEPT] metadata_calls_completed");
	}
	else
	{
		int ip[4];
		bool ipOk = SteamWorks_GetPublicIP(ip);
		PrintToServer("[SW-ACCEPT] status loaded=%d connected=%d vac=%d ipv4=%d completed=%d failed=%d headers=%d streamBytes=%d authEvents=%d",
			SteamWorks_IsLoaded(), SteamWorks_IsConnected(), SteamWorks_IsVACEnabled(), ipOk,
			g_Completed, g_Failed, g_Headers, g_StreamBytes, g_AuthEvents);
		PrintToServer("[SW-ACCEPT] validation_events current=%d legacy=%d", g_ValidationEvents, g_LegacyValidationEvents);
	}
	return Plugin_Handled;
}

public Action ReadStats(Handle timer, any account)
{
	int value = 0x12345678;
	float real = 1234.5;
	bool integerOk = SteamWorks_GetStatAuthIDCell(account, "Stat.GamesPlayed.Total", value);
	bool floatOk = SteamWorks_GetStatAuthIDFloat(account, "Stat.GamesPlayed.Total", real);
	PrintToServer("[SW-ACCEPT] authorized_stats_read integer=%d float=%d intFailurePreserved=%d floatFailurePreserved=%d",
		integerOk, floatOk, integerOk || value == 0x12345678, floatOk || real == 1234.5);
	for (int client = 1; client <= MaxClients; client++)
	{
		if (!IsClientInGame(client) || IsFakeClient(client) || !IsClientAuthorized(client)
			|| GetSteamAccountID(client, true) != account) continue;
		int clientValue = 0x12345678;
		float clientReal = 1234.5;
		bool clientIntegerOk = SteamWorks_GetStatCell(client, "Stat.GamesPlayed.Total", clientValue);
		bool clientFloatOk = SteamWorks_GetStatFloat(client, "Stat.GamesPlayed.Total", clientReal);
		PrintToServer("[SW-ACCEPT] client_stats_read integer=%d matchesAccount=%d float=%d floatFailurePreserved=%d",
			clientIntegerOk, clientIntegerOk && integerOk && clientValue == value,
			clientFloatOk, clientFloatOk || clientReal == 1234.5);
	}
	return Plugin_Stop;
}

void QueueRequest(int test)
{
	char url[128];
	switch (test)
	{
		case 0: strcopy(url, sizeof(url), "http://127.0.0.1:27880/text");
		case 1: strcopy(url, sizeof(url), "http://127.0.0.1:27880/binary");
		case 2: strcopy(url, sizeof(url), "http://127.0.0.1:27880/empty");
		case 3: strcopy(url, sizeof(url), "http://127.0.0.1:27880/error");
		case 4: strcopy(url, sizeof(url), "http://127.0.0.1:27880/delay");
		case 5: strcopy(url, sizeof(url), "https://127.0.0.1:27881/text");
		case 6: strcopy(url, sizeof(url), "http://127.0.0.1:27883/unreachable");
		case 7: strcopy(url, sizeof(url), "http://127.0.0.1:27880/stream");
		case 8: strcopy(url, sizeof(url), "http://localhost:27880/text");
	}
	Handle request = SteamWorks_CreateHTTPRequest(k_EHTTPMethodGET, url);
	if (request == null)
	{
		g_Failed++;
		PrintToServer("[SW-ACCEPT] create_failed case=%d", test);
		return;
	}
	bool contextOk = SteamWorks_SetHTTPRequestContextValue(request, test, 0x12345678);
	bool certificateOk = test != 5 || SteamWorks_SetHTTPRequestRequiresVerifiedCertificate(request, true);
	bool agentOk = SteamWorks_SetHTTPRequestUserAgentInfo(request, "SteamWorks-Acceptance");
	bool timeoutOk = SteamWorks_SetHTTPRequestAbsoluteTimeoutMS(request, test == 4 ? 500 : 5000);
	bool callbacksOk = SteamWorks_SetHTTPCallbacks(request, Completed, Headers, Data);
	bool sent = test == 7 ? SteamWorks_SendHTTPRequestAndStreamResponse(request) : SteamWorks_SendHTTPRequest(request);
	if (!sent)
	{
		g_Failed++;
		delete request;
	}
	PrintToServer("[SW-ACCEPT] queued case=%d sent=%d", test, sent);
	PrintToServer("[SW-ACCEPT] configured case=%d context=%d certificate=%d agent=%d timeout=%d callbacks=%d", test, contextOk, certificateOk, agentOk, timeoutOk, callbacksOk);
}

public void Headers(Handle request, bool failed, any test, any marker)
{
	g_Headers++;
	bool contextOk = test == 7 && marker == 0x12345678;
	if (failed || !contextOk) g_Failed++;
	PrintToServer("[SW-ACCEPT] headers case=%d failure=%d context=%d", test, failed, contextOk);
}

public void Data(Handle request, bool failed, int offset, int count, any test, any marker)
{
	char data[64];
	bool contextOk = test == 7 && marker == 0x12345678;
	bool ok = !failed && count >= 0 && count <= sizeof(data);
	if (ok) ok = SteamWorks_GetHTTPStreamingResponseBodyData(request, offset, data, count);
	int expected[6] = {'A', 0, 'B', 255, 0, 'Z'};
	if (ok && offset >= 0 && offset + count <= sizeof(expected))
	{
		for (int i = 0; i < count; i++) if ((data[i] & 255) != expected[offset + i]) ok = false;
	}
	else ok = false;
	if (!ok || !contextOk) g_Failed++;
	else g_StreamBytes += count;
	PrintToServer("[SW-ACCEPT] data case=%d payload=%d context=%d offset=%d bytes=%d", test, ok, contextOk, offset, count);
}

public void Completed(Handle request, bool failure, bool success, EHTTPStatusCode status, any test, any marker)
{
	if (test == 99 && g_Cancelled)
	{
		g_Failed++;
		PrintToServer("[SW-ACCEPT] FAIL cancelled_callback");
		return;
	}
	g_Completed++;
	bool ok = marker == 0x12345678;
	int size = -123;
	bool sizeOk = SteamWorks_GetHTTPResponseBodySize(request, size);
	bool timedOut;
	bool timeoutOk = SteamWorks_GetHTTPRequestWasTimedOut(request, timedOut);
	if (test == 4) ok = ok && (!success || failure) && timeoutOk && timedOut;
	else if (test == 5 || test == 6) ok = ok && (!success || failure);
	else
	{
		ok = ok && !failure && success && view_as<int>(status) == (test == 3 ? 503 : 200);
		if (test != 7)
		{
			ok = ok && sizeOk;
			if (test == 1)
			{
				char binary[6];
				bool read = SteamWorks_GetHTTPResponseBodyData(request, binary, sizeof(binary));
				ok = ok && read && size == 6 && binary[0] == 'A' && binary[1] == 0 && binary[2] == 'B'
					&& (binary[3] & 255) == 255 && binary[4] == 0 && binary[5] == 'Z';
			}
			else
			{
				char text[64];
				int written = -123;
				bool truncated = true;
				bool read = SteamWorks_GetHTTPResponseBodyString(request, text, sizeof(text), written, truncated);
				ok = ok && read && !truncated && written == size;
				if (test == 0 || test == 8) ok = ok && StrEqual(text, "steamworks-local");
				if (test == 2) ok = ok && size == 0 && text[0] == 0;
				if (test == 3) ok = ok && StrEqual(text, "fixture-error");
			}
		}
	}
	if (!ok) g_Failed++;
	PrintToServer("[SW-ACCEPT] complete case=%d pass=%d failure=%d success=%d status=%d sizeOk=%d size=%d timeoutOk=%d timeout=%d",
		test, ok, failure, success, status, sizeOk, size, timeoutOk, timedOut);
	// Real host regression: close the owning handle inside its callback.
	delete request;
}
