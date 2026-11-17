#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <sdktools>
#include <steamworks>

public Plugin myinfo = {name="SteamWorks Connection Observer", author="Lechuga", version="1.0.0"};

enum struct PluginRuntimeState
{
    bool lateLoad;
    bool bound;
    int owner;
    Handle getters[9];
    int baseline[9];
}
PluginRuntimeState g_Runtime;
Handle g_Watch, g_Pending, g_Retained;
bool g_RetainPending;
int g_LastConnected = -1, g_ConnectedEvents, g_DisconnectedEvents;
int g_Checks, g_Failures, g_HTTPCompleted;

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int length)
{
    g_Runtime.lateLoad = late;
    return APLRes_Success;
}

void Check(bool ok, const char[] name)
{
    g_Checks++;
    if (!ok) g_Failures++;
    PrintToServer("[SW-CONNECTION] check=%s pass=%d", name, ok);
}

public void OnPluginStart()
{
    RegAdminCmd("sm_sw_connection_bind", Bind, ADMFLAG_ROOT);
    RegAdminCmd("sm_sw_connection_check", Inspect, ADMFLAG_ROOT);
    RegAdminCmd("sm_sw_connection_http", QueueHTTP, ADMFLAG_ROOT);
    g_Watch = CreateTimer(1.0, Watch, _, TIMER_REPEAT);
    PrintToServer("[SW-CONNECTION] ready late=%d", g_Runtime.lateLoad);
}

public void OnPluginEnd()
{
    delete g_Watch;
    delete g_Pending;
    delete g_Retained;
    for (int i; i < sizeof(g_Runtime.getters); i++) delete g_Runtime.getters[i];
    PrintToServer("[SW-CONNECTION] end checks=%d failures=%d", g_Checks, g_Failures);
}

public Action Watch(Handle timer)
{
    int connected = SteamWorks_IsConnected() ? 1 : 0;
    if (connected != g_LastConnected)
    {
        g_LastConnected = connected;
        PrintToServer("[SW-CONNECTION] transition loaded=%d connected=%d connected_events=%d disconnected_events=%d",
            SteamWorks_IsLoaded(), connected, g_ConnectedEvents, g_DisconnectedEvents);
    }
    return Plugin_Continue;
}

public void SteamWorks_SteamServersConnected()
{
    g_ConnectedEvents++;
    PrintToServer("[SW-CONNECTION] SDK_connected count=%d", g_ConnectedEvents);
}

public void SteamWorks_SteamServersDisconnected(EResult reason)
{
    g_DisconnectedEvents++;
    PrintToServer("[SW-CONNECTION] SDK_disconnected count=%d reason=%d", g_DisconnectedEvents, reason);
}

int AddressArgument(int index)
{
    char text[16];
    GetCmdArg(index, text, sizeof(text));
    return StringToInt(text, 16);
}

public Action Bind(int client, int args)
{
    // Host validates exact mapped ELF, executable RVAs and writable owner slot.
    if (client != 0 || args != 10 || g_Runtime.bound) return Plugin_Handled;
    g_Runtime.owner = LoadFromAddress(view_as<Address>(AddressArgument(1)), NumberType_Int32);
    Check(g_Runtime.owner != 0, "resolver_present");
    if (g_Runtime.owner == 0) return Plugin_Handled;
    for (int i; i < sizeof(g_Runtime.getters); i++)
    {
        StartPrepSDKCall(SDKCall_Raw);
        PrepSDKCall_SetAddress(view_as<Address>(AddressArgument(i + 2)));
        PrepSDKCall_SetReturnInfo(SDKType_PlainOldData, SDKPass_Plain);
        g_Runtime.getters[i] = EndPrepSDKCall();
        Check(g_Runtime.getters[i] != null, "getter_prepared");
        if (g_Runtime.getters[i] == null) return Plugin_Handled;
        g_Runtime.baseline[i] = SDKCall(g_Runtime.getters[i], g_Runtime.owner);
        Check(i == 6 ? g_Runtime.baseline[i] == 0 : g_Runtime.baseline[i] != 0,
            "initial_profile_interface_availability");
    }
    g_Runtime.bound = true;
    PrintToServer("[SW-CONNECTION] bound=1 checks=%d failures=%d", g_Checks, g_Failures);
    return Plugin_Handled;
}

bool ReadRetained()
{
    if (g_Retained == null) return false;
    char body[17];
    int size = -1, written = -1;
    bool truncated = true;
    return SteamWorks_GetHTTPResponseBodySize(g_Retained, size) && size == 16
        && SteamWorks_GetHTTPResponseBodyString(g_Retained, body, sizeof(body), written, truncated)
        && written == 16 && !truncated && StrEqual(body, "steamworks-local");
}

public Action Inspect(int client, int args)
{
    if (client != 0 || args != 1) return Plugin_Handled;
    char mode[16];
    GetCmdArg(1, mode, sizeof(mode));
    if (!StrEqual(mode, "baseline") && !StrEqual(mode, "down") && !StrEqual(mode, "up")) return Plugin_Handled;
    bool down = StrEqual(mode, "down");
    Check(SteamWorks_IsLoaded(), "runtime_remains_loaded");
    Check(SteamWorks_IsConnected() != down, "connection_matches_phase");
    Check(g_Runtime.bound, "getters_bound");
    if (g_Runtime.bound)
        for (int i; i < sizeof(g_Runtime.getters); i++)
            Check(SDKCall(g_Runtime.getters[i], g_Runtime.owner) == g_Runtime.baseline[i], "same_interface_pointer");
    Check(ReadRetained(), "retained_HTTP_response_readable");
    int value = 0x13572468;
    Check(!SteamWorks_GetStatAuthIDCell(0, "__steamworks_connection_missing__", value)
        && value == 0x13572468, "invalid_stats_preserve_output");
    if (down) Check(g_DisconnectedEvents > 0, "real_SDK_disconnect_forward_seen");
    if (StrEqual(mode, "up")) Check(g_ConnectedEvents > 1, "real_SDK_reconnect_forward_seen");
    PrintToServer("[SW-CONNECTION] phase=%s checks=%d failures=%d HTTP_completed=%d connected_events=%d disconnected_events=%d",
        mode, g_Checks, g_Failures, g_HTTPCompleted, g_ConnectedEvents, g_DisconnectedEvents);
    return Plugin_Handled;
}

public Action QueueHTTP(int client, int args)
{
    if (client != 0 || args != 1 || g_Pending != null) return Plugin_Handled;
    char mode[16];
    GetCmdArg(1, mode, sizeof(mode));
    g_RetainPending = StrEqual(mode, "retain");
    if ((!g_RetainPending && !StrEqual(mode, "fresh")) || (g_RetainPending && g_Retained != null)) return Plugin_Handled;
    g_Pending = SteamWorks_CreateHTTPRequest(k_EHTTPMethodGET, "http://127.0.0.1:27880/text");
    Check(g_Pending != null, "HTTP_created");
    if (g_Pending == null) return Plugin_Handled;
    bool configured = SteamWorks_SetHTTPRequestContextValue(g_Pending, 31, 0x12345678)
        && SteamWorks_SetHTTPCallbacks(g_Pending, Completed);
    Check(configured, "HTTP_context_callbacks_configured");
    bool sent = configured && SteamWorks_SendHTTPRequest(g_Pending);
    Check(sent, "HTTP_sent");
    if (!sent) delete g_Pending;
    return Plugin_Handled;
}

public void Completed(Handle request, bool failed, bool success, EHTTPStatusCode status, any high, any low)
{
    g_HTTPCompleted++;
    char body[17];
    int written = -1;
    bool truncated = true;
    Check(request == g_Pending && !failed && success && view_as<int>(status) == 200, "HTTP_completion_success");
    Check(high == 31 && low == 0x12345678, "HTTP_context_preserved");
    Check(SteamWorks_GetHTTPResponseBodyString(request, body, sizeof(body), written, truncated)
        && written == 16 && !truncated && StrEqual(body, "steamworks-local"), "HTTP_body_preserved");
    if (g_RetainPending) { g_Retained = g_Pending; g_Pending = null; }
    else delete g_Pending;
    PrintToServer("[SW-CONNECTION] HTTP_complete count=%d retained=%d checks=%d failures=%d",
        g_HTTPCompleted, g_Retained != null, g_Checks, g_Failures);
}
