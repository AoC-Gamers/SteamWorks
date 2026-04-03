#include <sourcemod>
#include <SteamWorks>

#define SWTEST_VERSION "1.0.0"

public Plugin myinfo =
{
	name = "SteamWorks Test",
	author = "OpenAI",
	description = "Grouped test commands and forward logging for SteamWorks",
	version = SWTEST_VERSION,
	url = ""
};

static const int PERSONA_POLL_MAX_ATTEMPTS = 10;
static const float PERSONA_POLL_INTERVAL = 1.0;

public void OnPluginStart()
{
	RegAdminCmd("sm_swtest_http", Cmd_TestHTTP, ADMFLAG_GENERIC, "Tests SteamWorks HTTP support. Usage: sm_swtest_http [url]");
	RegAdminCmd("sm_swtest_persona", Cmd_TestPersona, ADMFLAG_GENERIC, "Tests persona natives. Usage: sm_swtest_persona <target|accountid> [nameonly 0|1]");
	RegAdminCmd("sm_swtest_clan", Cmd_TestClan, ADMFLAG_GENERIC, "Tests clan officer natives. Usage: sm_swtest_clan <groupid>");
	RegAdminCmd("sm_swtest_stats", Cmd_TestStats, ADMFLAG_GENERIC, "Tests stat natives. Usage: sm_swtest_stats <target>");
	RegAdminCmd("sm_swtest_profile", Cmd_TestProfile, ADMFLAG_GENERIC, "Tests profile/friends natives. Usage: sm_swtest_profile <target>");

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

static bool SWTest_ResolveTargetOrAccountId(int issuer, const char[] input, int &target, int &accountid)
{
	target = 0;
	accountid = 0;

	int parsed = StringToInt(input);
	if (parsed > 0)
	{
		accountid = parsed;
		return true;
	}

	target = FindTarget(issuer, input, true, false);
	if (target <= 0)
	{
		return false;
	}

	accountid = GetSteamAccountID(target, false);
	return (accountid > 0);
}

static void SWTest_LogPersonaSnapshot(int issuer, int accountid)
{
	char persona[128];
	char nickname[128];
	int state = SteamWorks_GetFriendPersonaStateAuthID(accountid);
	int relationship = SteamWorks_GetFriendRelationshipAuthID(accountid);
	int personaWritten = SteamWorks_GetFriendPersonaNameAuthID(accountid, persona, sizeof(persona));
	int nicknameWritten = SteamWorks_GetPlayerNicknameAuthID(accountid, nickname, sizeof(nickname));

	SWTest_LogCommand(issuer,
		"persona snapshot accountid=%d state=%d relationship=%d persona=%s nickname=%s",
		accountid,
		state,
		relationship,
		(personaWritten > 0 && persona[0] != '\0') ? persona : "<empty>",
		(nicknameWritten > 0 && nickname[0] != '\0') ? nickname : "<empty>");
}

static void SWTest_StartPersonaPolling(int issuer, int accountid)
{
	DataPack pack = new DataPack();
	pack.WriteCell(GetClientUserId(issuer));
	pack.WriteCell(accountid);
	pack.WriteCell(1);
	CreateTimer(PERSONA_POLL_INTERVAL, Timer_PollPersona, pack, TIMER_FLAG_NO_MAPCHANGE);
}

public Action Timer_PollPersona(Handle timer, DataPack pack)
{
	pack.Reset();
	int issuerUserId = pack.ReadCell();
	int accountid = pack.ReadCell();
	int attempt = pack.ReadCell();
	int issuer = GetClientOfUserId(issuerUserId);

	char persona[128];
	int written = SteamWorks_GetFriendPersonaNameAuthID(accountid, persona, sizeof(persona));
	if (written > 0 && persona[0] != '\0')
	{
		SWTest_LogCommand(issuer, "persona poll resolved accountid=%d attempt=%d persona=%s", accountid, attempt, persona);
		delete pack;
		return Plugin_Stop;
	}

	SWTest_LogCommand(issuer, "persona poll pending accountid=%d attempt=%d", accountid, attempt);
	if (attempt >= PERSONA_POLL_MAX_ATTEMPTS)
	{
		delete pack;
		return Plugin_Stop;
	}

	DataPack nextPack = new DataPack();
	nextPack.WriteCell(issuerUserId);
	nextPack.WriteCell(accountid);
	nextPack.WriteCell(attempt + 1);
	CreateTimer(PERSONA_POLL_INTERVAL, Timer_PollPersona, nextPack, TIMER_FLAG_NO_MAPCHANGE);

	delete pack;
	return Plugin_Stop;
}

public Action Cmd_TestHTTP(int client, int args)
{
	char url[256];
	if (args >= 1)
	{
		GetCmdArg(1, url, sizeof(url));
	}
	else
	{
		strcopy(url, sizeof(url), "http://127.0.0.1:18080/health");
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

public void OnHTTPCompleted(Handle request, bool failure, bool success, EHTTPStatusCode code, any issuer)
{
	SWTest_LogCommand(issuer, "http completed failure=%d success=%d code=%d", failure, success, code);

	int size;
	if (SteamWorks_GetHTTPResponseBodySize(request, size) && size > 0)
	{
		char body[256];
		SteamWorks_GetHTTPResponseBodyData(request, body, sizeof(body));
		SWTest_LogCommand(issuer, "http body=%s", body);
	}

	CloseHandle(request);
}

public Action Cmd_TestPersona(int client, int args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SteamWorks Test] Usage: sm_swtest_persona <target|accountid> [nameonly 0|1]");
		return Plugin_Handled;
	}

	char arg[64];
	GetCmdArg(1, arg, sizeof(arg));

	int target, accountid;
	if (!SWTest_ResolveTargetOrAccountId(client, arg, target, accountid))
	{
		ReplyToCommand(client, "[SteamWorks Test] Invalid target or accountid.");
		return Plugin_Handled;
	}

	bool nameOnly = true;
	if (args >= 2)
	{
		GetCmdArg(2, arg, sizeof(arg));
		nameOnly = StringToInt(arg) != 0;
	}

	bool started = (target > 0)
		? SteamWorks_RequestUserInformation(target, nameOnly)
		: SteamWorks_RequestUserInformationAuthID(accountid, nameOnly);

	SWTest_LogCommand(client, "persona request accountid=%d target=%d nameonly=%d started=%d", accountid, target, nameOnly, started);
	SWTest_LogPersonaSnapshot(client, accountid);
	SWTest_StartPersonaPolling(client, accountid);
	return Plugin_Handled;
}

public Action Cmd_TestProfile(int client, int args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SteamWorks Test] Usage: sm_swtest_profile <target>");
		return Plugin_Handled;
	}

	char pattern[64];
	GetCmdArg(1, pattern, sizeof(pattern));

	int target = FindTarget(client, pattern, true, false);
	if (target <= 0)
	{
		return Plugin_Handled;
	}

	int appid, ip, gamePort, queryPort, lobbyAuthid;
	bool hasGame = SteamWorks_GetFriendGamePlayed(target, appid, ip, gamePort, queryPort, lobbyAuthid);

	char nickname[128];
	int nickWritten = SteamWorks_GetPlayerNickname(target, nickname, sizeof(nickname));
	SWTest_LogCommand(client,
		"profile target=%N accountid=%d state=%d relationship=%d nickname=%s game=%d appid=%d ip=%d gamePort=%d queryPort=%d lobby=%d",
		target,
		GetSteamAccountID(target, false),
		SteamWorks_GetFriendPersonaState(target),
		SteamWorks_GetFriendRelationship(target),
		(nickWritten > 0 && nickname[0] != '\0') ? nickname : "<empty>",
		hasGame,
		appid,
		ip,
		gamePort,
		queryPort,
		lobbyAuthid);
	return Plugin_Handled;
}

public Action Cmd_TestClan(int client, int args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SteamWorks Test] Usage: sm_swtest_clan <groupid>");
		return Plugin_Handled;
	}

	char arg[32];
	GetCmdArg(1, arg, sizeof(arg));
	int groupid = StringToInt(arg);
	if (groupid <= 0)
	{
		ReplyToCommand(client, "[SteamWorks Test] Invalid groupid.");
		return Plugin_Handled;
	}

	bool started = SteamWorks_RequestClanOfficerList(groupid);
	int count = SteamWorks_GetClanOfficerCount(groupid);
	SWTest_LogCommand(client, "clan request groupid=%d started=%d cachedCount=%d", groupid, started, count);

	if (count > 0)
	{
		for (int i = 0; i < count; i++)
		{
			SWTest_LogCommand(client, "clan officer[%d]=%d", i, SteamWorks_GetClanOfficerByIndex(groupid, i));
		}
	}

	return Plugin_Handled;
}

public Action Cmd_TestStats(int client, int args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SteamWorks Test] Usage: sm_swtest_stats <target>");
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
	bool clientReq = SteamWorks_RequestStats(target, 550);
	bool authReq = SteamWorks_RequestStatsAuthID(accountid, 550);
	SWTest_LogCommand(client, "stats target=%N accountid=%d clientReq=%d authReq=%d", target, accountid, clientReq, authReq);
	return Plugin_Handled;
}

public void SteamWorks_SteamServersConnected()
{
	PrintToServer("[SteamWorks Test] forward SteamServersConnected loaded=%d connected=%d vac=%d", SteamWorks_IsLoaded(), SteamWorks_IsConnected(), SteamWorks_IsVACEnabled());
}

public void SteamWorks_SteamServersConnectFailure(EResult result)
{
	PrintToServer("[SteamWorks Test] forward SteamServersConnectFailure result=%d", result);
}

public void SteamWorks_SteamServersDisconnected(EResult result)
{
	PrintToServer("[SteamWorks Test] forward SteamServersDisconnected result=%d", result);
}

public Action SteamWorks_RestartRequested()
{
	PrintToServer("[SteamWorks Test] forward RestartRequested");
	return Plugin_Continue;
}

public void SteamWorks_TokenRequested(char[] token, int maxlen)
{
	strcopy(token, maxlen, "STEAMWORKS_TEST_TOKEN");
	PrintToServer("[SteamWorks Test] forward TokenRequested");
}

public void SteamWorks_OnClientGroupStatus(int authid, int groupid, bool isMember, bool isOfficer)
{
	PrintToServer("[SteamWorks Test] forward ClientGroupStatus authid=%d groupid=%d member=%d officer=%d", authid, groupid, isMember, isOfficer);
}

public void SteamWorks_OnPersonaStateChange(int authid, int flags)
{
	PrintToServer("[SteamWorks Test] forward PersonaStateChange authid=%d flags=%d", authid, flags);
}

public void SteamWorks_OnUserInformationRequested(int authid, bool started)
{
	PrintToServer("[SteamWorks Test] forward UserInformationRequested authid=%d started=%d", authid, started);
}
