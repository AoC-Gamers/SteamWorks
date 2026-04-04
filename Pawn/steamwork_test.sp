#include <sourcemod>
#include <steamworks>

#define SWTEST_VERSION "1.0.0"
#define SWTEST_STATS_POLL_DELAY 2.0

public Plugin myinfo =
{
	name = "SteamWorks Test",
	author = "lechuga",
	description = "Grouped test commands and forward logging for SteamWorks",
	version = SWTEST_VERSION,
	url = ""
};

public void OnPluginStart()
{
	RegAdminCmd("sm_swtest_status", Cmd_TestStatus, ADMFLAG_GENERIC, "Tests SteamWorks status natives.");
	RegAdminCmd("sm_swtest_http", Cmd_TestHTTP, ADMFLAG_GENERIC, "Tests SteamWorks HTTP support. Usage: sm_swtest_http [url]");
	RegAdminCmd("sm_swtest_identity", Cmd_TestIdentity, ADMFLAG_GENERIC, "Tests Steam identity natives. Usage: sm_swtest_identity <target>");
	RegAdminCmd("sm_swtest_group", Cmd_TestGroup, ADMFLAG_GENERIC, "Tests Steam group status natives. Usage: sm_swtest_group <target> <groupid>");
	RegAdminCmd("sm_swtest_license", Cmd_TestLicense, ADMFLAG_GENERIC, "Tests license natives. Usage: sm_swtest_license <target> [appid]");
	RegAdminCmd("sm_swtest_servermeta", Cmd_TestServerMeta, ADMFLAG_GENERIC, "Tests server metadata natives.");
	RegAdminCmd("sm_swtest_stats", Cmd_TestStats, ADMFLAG_GENERIC, "Tests stat natives. Usage: sm_swtest_stats <target> [stat_key]");

	PrintToServer("[SteamWorks Test] Loaded version %s", SWTEST_VERSION);
}

static void SWTest_LogCommand(int client, const char[] fmt, any ...)
{
	char buffer[256];
	VFormat(buffer, sizeof(buffer), fmt, 3);

	PrintToServer("[SteamWorks Test] %s", buffer);
	if (client > 0 && IsClientInGame(client))
	{
		ReplyToCommand(client, "[SteamWorks Test] %s", buffer);
	}
}

static void SWTest_LogEvent(const char[] fmt, any ...)
{
	char buffer[256];
	VFormat(buffer, sizeof(buffer), fmt, 2);

	PrintToServer("[SteamWorks Test] %s", buffer);
	LogMessage("[SteamWorks Test] %s", buffer);
}

static void SWTest_LogHttpBody(int size, const char[] body)
{
	LogMessage("[SteamWorks Test] http body size=%d", size);
	LogMessage("[SteamWorks Test] http body begin");
	LogMessage("[SteamWorks Test] %s", body);
	LogMessage("[SteamWorks Test] http body end");
}

static bool SWTest_ResolveTarget(int issuer, const char[] pattern, int &target)
{
	target = FindTarget(issuer, pattern, true, false);
	return (target > 0);
}

static void SWTest_LogStatRead(int client, int target, int accountid, const char[] key)
{
	int clientCellValue = 0;
	int authCellValue = 0;
	float clientFloatValue = 0.0;
	float authFloatValue = 0.0;

	bool clientCellOk = SteamWorks_GetStatCell(target, key, clientCellValue);
	bool authCellOk = SteamWorks_GetStatAuthIDCell(accountid, key, authCellValue);
	bool clientFloatOk = SteamWorks_GetStatFloat(target, key, clientFloatValue);
	bool authFloatOk = SteamWorks_GetStatAuthIDFloat(accountid, key, authFloatValue);

	SWTest_LogCommand(
		client,
		"stats key=%s clientCellOk=%d clientCell=%d authCellOk=%d authCell=%d clientFloatOk=%d clientFloat=%.3f authFloatOk=%d authFloat=%.3f",
		key,
		clientCellOk,
		clientCellValue,
		authCellOk,
		authCellValue,
		clientFloatOk,
		clientFloatValue,
		authFloatOk,
		authFloatValue
	);
}

public Action Timer_PollStats(Handle timer, DataPack pack)
{
	pack.Reset();
	int issuerUserId = pack.ReadCell();
	int targetUserId = pack.ReadCell();
	int accountid = pack.ReadCell();
	char key[128];
	pack.ReadString(key, sizeof(key));

	int client = GetClientOfUserId(issuerUserId);
	int target = GetClientOfUserId(targetUserId);

	if (target <= 0 || !IsClientInGame(target))
	{
		SWTest_LogCommand(client, "stats poll target left before lookup key=%s accountid=%d", key, accountid);
		delete pack;
		return Plugin_Stop;
	}

	SWTest_LogCommand(client, "stats poll target=%N accountid=%d key=%s", target, accountid, key);
	SWTest_LogStatRead(client, target, accountid, key);

	delete pack;
	return Plugin_Stop;
}

public Action Cmd_TestHTTP(int client, int args)
{
	char url[256];
	if (args >= 1)
	{
		GetCmdArgString(url, sizeof(url));
		TrimString(url);

		int length = strlen(url);
		if (length >= 2 && url[0] == '"' && url[length - 1] == '"')
		{
			url[length - 1] = '\0';
			for (int i = 1; i < length; i++)
			{
				url[i - 1] = url[i];
			}
		}

		if (StrEqual(url, "http:") || StrEqual(url, "https:"))
		{
			SWTest_LogCommand(client, "the Source console truncates URLs on //, use quotes or omit the scheme");
			return Plugin_Handled;
		}

		if (StrContains(url, "://") == -1)
		{
			char normalized[256];
			Format(normalized, sizeof(normalized), "https://%s", url);
			strcopy(url, sizeof(url), normalized);
		}
	}
	else
	{
		strcopy(url, sizeof(url), "https://example.com/");
	}

	Handle request = SteamWorks_CreateHTTPRequest(k_EHTTPMethodGET, url);
	if (request == INVALID_HANDLE)
	{
		SWTest_LogCommand(client, "http create failed url=%s", url);
		return Plugin_Handled;
	}

	SteamWorks_SetHTTPRequestContextValue(request, client);
	SteamWorks_SetHTTPRequestHeaderValue(request, "User-Agent", "SteamWorks-Test");
	SteamWorks_SetHTTPCallbacks(request, OnHTTPCompleted);

	if (!SteamWorks_SendHTTPRequest(request))
	{
		SWTest_LogCommand(client, "http send failed url=%s", url);
		CloseHandle(request);
		return Plugin_Handled;
	}

	SWTest_LogCommand(client, "http queued url=%s", url);
	return Plugin_Handled;
}

public Action Cmd_TestStatus(int client, int args)
{
	int ip[4];
	bool ipOk = SteamWorks_GetPublicIP(ip);
	int ipCell = SteamWorks_GetPublicIPCell();

	SWTest_LogCommand(
		client,
		"status loaded=%d connected=%d vac=%d ipOk=%d ip=%d.%d.%d.%d ipCell=%d",
		SteamWorks_IsLoaded(),
		SteamWorks_IsConnected(),
		SteamWorks_IsVACEnabled(),
		ipOk,
		ip[0],
		ip[1],
		ip[2],
		ip[3],
		ipCell
	);

	return Plugin_Handled;
}

public void OnHTTPCompleted(Handle request, bool failure, bool success, EHTTPStatusCode code, any issuer)
{
	SWTest_LogCommand(issuer, "http completed failure=%d success=%d code=%d", failure, success, code);

	int size;
	if (SteamWorks_GetHTTPResponseBodySize(request, size) && size > 0)
	{
		SWTest_LogCommand(issuer, "http body size=%d", size);

		char body[2048];
		int readLength = size;
		if (readLength > sizeof(body) - 1)
		{
			readLength = sizeof(body) - 1;
		}

		if (SteamWorks_GetHTTPResponseBodyData(request, body, readLength))
		{
			body[readLength] = '\0';
			SWTest_LogHttpBody(size, body);
			if (size <= 200)
			{
				SWTest_LogCommand(issuer, "http body=%s", body);
			}
			else
			{
				char preview[256];
				int previewLength = strlen(body);
				if (previewLength > sizeof(preview) - 1)
				{
					previewLength = sizeof(preview) - 1;
				}

				for (int i = 0; i < previewLength; i++)
				{
					preview[i] = body[i];
				}
				preview[previewLength] = '\0';

				SWTest_LogCommand(issuer, "http body preview=%s", preview);
				SWTest_LogCommand(issuer, "http body truncated for console output");
			}
		}
		else
		{
			SWTest_LogCommand(issuer, "http body read failed");
		}
	}
	else
	{
		SWTest_LogCommand(issuer, "http body size=0");
	}

	CloseHandle(request);
}

public Action Cmd_TestIdentity(int client, int args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SteamWorks Test] Usage: sm_swtest_identity <target>");
		return Plugin_Handled;
	}

	char pattern[64];
	GetCmdArg(1, pattern, sizeof(pattern));

	int target;
	if (!SWTest_ResolveTarget(client, pattern, target))
	{
		return Plugin_Handled;
	}

	char steamId[64];
	int written = SteamWorks_GetClientSteamID(target, steamId, sizeof(steamId));
	SWTest_LogCommand(
		client,
		"identity target=%N userid=%d accountid=%d written=%d steamid=%s",
		target,
		GetClientUserId(target),
		GetSteamAccountID(target, false),
		written,
		(written > 0 && steamId[0] != '\0') ? steamId : "<empty>"
	);

	return Plugin_Handled;
}

public Action Cmd_TestGroup(int client, int args)
{
	if (args < 2)
	{
		ReplyToCommand(client, "[SteamWorks Test] Usage: sm_swtest_group <target> <groupid>");
		return Plugin_Handled;
	}

	char pattern[64];
	char groupArg[32];
	GetCmdArg(1, pattern, sizeof(pattern));
	GetCmdArg(2, groupArg, sizeof(groupArg));

	int groupid = StringToInt(groupArg);
	if (groupid <= 0)
	{
		ReplyToCommand(client, "[SteamWorks Test] Invalid groupid.");
		return Plugin_Handled;
	}

	int target;
	if (!SWTest_ResolveTarget(client, pattern, target))
	{
		return Plugin_Handled;
	}

	int accountid = GetSteamAccountID(target, false);
	bool byClient = SteamWorks_GetUserGroupStatus(target, groupid);
	bool byAuthid = SteamWorks_GetUserGroupStatusAuthID(accountid, groupid);
	SWTest_LogCommand(
		client,
		"group target=%N accountid=%d groupid=%d byClient=%d byAuthid=%d",
		target,
		accountid,
		groupid,
		byClient,
		byAuthid
	);

	return Plugin_Handled;
}

public Action Cmd_TestLicense(int client, int args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SteamWorks Test] Usage: sm_swtest_license <target> [appid]");
		return Plugin_Handled;
	}

	char pattern[64];
	char appArg[32];
	GetCmdArg(1, pattern, sizeof(pattern));

	int appid = 550;
	if (args >= 2)
	{
		GetCmdArg(2, appArg, sizeof(appArg));
		int parsed = StringToInt(appArg);
		if (parsed > 0)
		{
			appid = parsed;
		}
	}

	int target;
	if (!SWTest_ResolveTarget(client, pattern, target))
	{
		return Plugin_Handled;
	}

	int accountid = GetSteamAccountID(target, false);
	EUserHasLicenseForAppResult byClient = SteamWorks_HasLicenseForApp(target, appid);
	EUserHasLicenseForAppResult byAuthid = SteamWorks_HasLicenseForAppId(accountid, appid);
	SWTest_LogCommand(
		client,
		"license target=%N accountid=%d appid=%d byClient=%d byAuthid=%d",
		target,
		accountid,
		appid,
		byClient,
		byAuthid
	);

	return Plugin_Handled;
}

public Action Cmd_TestServerMeta(int client, int args)
{
	char mapName[64];
	GetCurrentMap(mapName, sizeof(mapName));

	bool gameDataOk = SteamWorks_SetGameData("steamworks_test=1");
	bool gameDescriptionOk = SteamWorks_SetGameDescription("SteamWorks Test Description");
	bool mapNameOk = SteamWorks_SetMapName(mapName);
	bool setRuleOk = SteamWorks_SetRule("steamworks_test_rule", "1");
	bool clearRulesOk = SteamWorks_ClearRules();

	SWTest_LogCommand(
		client,
		"servermeta gameDataOk=%d gameDescriptionOk=%d mapNameOk=%d setRuleOk=%d clearRulesOk=%d map=%s",
		gameDataOk,
		gameDescriptionOk,
		mapNameOk,
		setRuleOk,
		clearRulesOk,
		mapName
	);

	return Plugin_Handled;
}

public Action Cmd_TestStats(int client, int args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SteamWorks Test] Usage: sm_swtest_stats <target> [stat_key]");
		ReplyToCommand(client, "[SteamWorks Test] Example: sm_swtest_stats veri Stat.GamesPlayed.Total");
		return Plugin_Handled;
	}

	char pattern[64];
	GetCmdArg(1, pattern, sizeof(pattern));

	int target = FindTarget(client, pattern, true, false);
	if (target <= 0)
	{
		return Plugin_Handled;
	}

	int accountid = GetSteamAccountID(target, false);
	int userid = GetClientUserId(target);
	bool clientReq = SteamWorks_RequestStats(target, 550);
	bool authReq = SteamWorks_RequestStatsAuthID(accountid, 550);
	SWTest_LogCommand(client, "stats target=%N userid=%d accountid=%d appid=550 clientReq=%d authReq=%d", target, userid, accountid, clientReq, authReq);

	if (args >= 2)
	{
		char key[128];
		GetCmdArg(2, key, sizeof(key));
		SWTest_LogCommand(client, "stats immediate read key=%s", key);
		SWTest_LogStatRead(client, target, accountid, key);

		DataPack pack = new DataPack();
		pack.WriteCell((client > 0) ? GetClientUserId(client) : 0);
		pack.WriteCell(userid);
		pack.WriteCell(accountid);
		pack.WriteString(key);
		CreateTimer(SWTEST_STATS_POLL_DELAY, Timer_PollStats, pack, TIMER_FLAG_NO_MAPCHANGE);
	}
	else
	{
		SWTest_LogCommand(client, "stats request sent; pass a stat key as second argument to test reads");
		SWTest_LogCommand(client, "example: sm_swtest_stats \"%N\" Stat.GamesPlayed.Total", target);
	}
	return Plugin_Handled;
}

public void SteamWorks_SteamServersConnected()
{
	SWTest_LogEvent("forward SteamServersConnected loaded=%d connected=%d vac=%d", SteamWorks_IsLoaded(), SteamWorks_IsConnected(), SteamWorks_IsVACEnabled());
}

public void SteamWorks_SteamServersConnectFailure(EResult result)
{
	SWTest_LogEvent("forward SteamServersConnectFailure result=%d", result);
}

public void SteamWorks_SteamServersDisconnected(EResult result)
{
	SWTest_LogEvent("forward SteamServersDisconnected result=%d", result);
}

public Action SteamWorks_RestartRequested()
{
	SWTest_LogEvent("forward RestartRequested");
	return Plugin_Continue;
}

public void SteamWorks_TokenRequested(char[] token, int maxlen)
{
	strcopy(token, maxlen, "STEAMWORKS_TEST_TOKEN");
	SWTest_LogEvent("forward TokenRequested");
}

public void SteamWorks_OnClientGroupStatus(int authid, int groupid, bool isMember, bool isOfficer)
{
	SWTest_LogEvent("forward ClientGroupStatus authid=%d groupid=%d member=%d officer=%d", authid, groupid, isMember, isOfficer);
}
