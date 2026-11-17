#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <steamworks>

#define SOAK_SLOTS 8
#define SOAK_CONTEXT 0x6C2A1293

public Plugin myinfo = {name="SteamWorks Soak Observer", author="Lechuga", version="1.0.0"};

Handle g_Work;
Handle g_Requests[SOAK_SLOTS];
int g_Sequence[SOAK_SLOTS], g_Kind[SOAK_SLOTS], g_StreamMask[SOAK_SLOTS];
bool g_Headers[SOAK_SLOTS];
float g_SentAt[SOAK_SLOTS];
float g_Until, g_PreviousFrame, g_FrameSum, g_FrameMax;
int g_Frames, g_Sent, g_Completed, g_Closed, g_Failures, g_Pending, g_Peak;
int g_KindCompleted[4], g_LatencyBins[4][6], g_StreamBytes, g_StreamCallbacks;
float g_LatencySum[4], g_LatencyMax[4];
bool g_Monitoring = true;

public void OnPluginStart()
{
    RegAdminCmd("sm_sw_soak", Control, ADMFLAG_ROOT);
    PrintToServer("[SW-SOAK] ready");
}

public void OnPluginEnd()
{
    delete g_Work;
    for (int i; i < SOAK_SLOTS; i++) delete g_Requests[i];
    PrintToServer("[SW-SOAK] end sent=%d completed=%d closed=%d pending=%d failures=%d",
        g_Sent, g_Completed, g_Closed, g_Pending, g_Failures);
}

public void OnMapEnd()
{
    delete g_Work;
    for (int i; i < SOAK_SLOTS; i++) delete g_Requests[i];
    if (g_Monitoring)
    {
        g_Failures++;
        PrintToServer("[SW-SOAK] unexpected_map_end=1");
    }
}

public void OnGameFrame()
{
    float now = GetEngineTime();
    if (g_PreviousFrame > 0.0)
    {
        float delta = now - g_PreviousFrame;
        g_FrameSum += delta;
        if (delta > g_FrameMax) g_FrameMax = delta;
        g_Frames++;
    }
    g_PreviousFrame = now;
}

void Failed()
{
    g_Failures++;
    // Bound failure logging independently of request/frame rate.
    if (g_Failures <= 3) PrintToServer("[SW-SOAK] failure count=%d", g_Failures);
}

public Action Control(int client, int args)
{
    if (client != 0 || args != 1) return Plugin_Handled;
    char mode[24];
    GetCmdArg(1, mode, sizeof(mode));
    if (StrEqual(mode, "burst"))
    {
        if (g_Work != null || g_Pending != 0) { Failed(); return Plugin_Handled; }
        g_Until = GetEngineTime() + 50.0;
        g_Work = CreateTimer(0.5, Launch, _, TIMER_REPEAT);
        PrintToServer("[SW-SOAK] burst_started duration=50 interval=0.5");
    }
    else
    {
        if (StrEqual(mode, "finish"))
        {
            if (g_Pending != 0 || g_Work != null) Failed();
            else g_Monitoring = false;
        }
        if (!SteamWorks_IsLoaded() || !SteamWorks_IsConnected()) Failed();
        float mean = g_Frames > 0 ? 1000.0 * g_FrameSum / float(g_Frames) : 0.0;
        PrintToServer("[SW-SOAK] status phase=%s sent=%d completed=%d closed=%d pending=%d peak=%d failures=%d loaded=%d connected=%d frames=%d frame_mean_ms=%.3f frame_max_ms=%.3f stream_bytes=%d stream_callbacks=%d working=%d",
            mode, g_Sent, g_Completed, g_Closed, g_Pending, g_Peak, g_Failures,
            SteamWorks_IsLoaded(), SteamWorks_IsConnected(), g_Frames, mean,
            1000.0 * g_FrameMax, g_StreamBytes, g_StreamCallbacks, g_Work != null);
        for (int kind; kind < 4; kind++)
            PrintToServer("[SW-SOAK] latency kind=%d completed=%d sum_ms=%.3f max_ms=%.3f bins=%d,%d,%d,%d,%d,%d",
                kind, g_KindCompleted[kind], g_LatencySum[kind], g_LatencyMax[kind],
                g_LatencyBins[kind][0], g_LatencyBins[kind][1], g_LatencyBins[kind][2],
                g_LatencyBins[kind][3], g_LatencyBins[kind][4], g_LatencyBins[kind][5]);
        g_Frames = 0;
        g_FrameSum = 0.0;
        g_FrameMax = 0.0;
    }
    return Plugin_Handled;
}

public Action Launch(Handle timer)
{
    if (GetEngineTime() >= g_Until) { g_Work = null; return Plugin_Stop; }
    int slot = -1;
    for (int i; i < SOAK_SLOTS; i++) if (g_Requests[i] == null) { slot = i; break; }
    if (slot == -1) { Failed(); return Plugin_Continue; }
    g_Sent++;
    int kind = (g_Sent - 1) % 4;
    char path[16], url[80];
    switch (kind)
    {
        case 0: strcopy(path, sizeof(path), "text");
        case 1: strcopy(path, sizeof(path), "stream");
        case 2: strcopy(path, sizeof(path), "error");
        case 3: strcopy(path, sizeof(path), "delay");
    }
    Format(url, sizeof(url), "http://127.0.0.1:27880/%s", path);
    g_Requests[slot] = SteamWorks_CreateHTTPRequest(k_EHTTPMethodGET, url);
    if (g_Requests[slot] == null) { Failed(); return Plugin_Continue; }
    g_Sequence[slot] = g_Sent;
    g_Kind[slot] = kind;
    g_StreamMask[slot] = 0;
    g_Headers[slot] = false;
    g_SentAt[slot] = GetEngineTime();
    g_Pending++;
    if (g_Pending > g_Peak) g_Peak = g_Pending;
    bool configured = SteamWorks_SetHTTPRequestContextValue(g_Requests[slot], g_Sent, SOAK_CONTEXT)
        && SteamWorks_SetHTTPRequestAbsoluteTimeoutMS(g_Requests[slot], 10000);
    if (kind == 1) configured = configured && SteamWorks_SetHTTPCallbacks(g_Requests[slot], Completed, Headers, Data);
    else configured = configured && SteamWorks_SetHTTPCallbacks(g_Requests[slot], Completed);
    bool sent = false;
    if (configured) sent = kind == 1 ? SteamWorks_SendHTTPRequestAndStreamResponse(g_Requests[slot]) : SteamWorks_SendHTTPRequest(g_Requests[slot]);
    if (!sent) { Failed(); CloseSlot(slot); }
    return Plugin_Continue;
}

int FindSlot(Handle request, int sequence, int marker)
{
    for (int i; i < SOAK_SLOTS; i++)
        if (g_Requests[i] == request && g_Sequence[i] == sequence && marker == SOAK_CONTEXT) return i;
    Failed();
    return -1;
}

void CloseSlot(int slot)
{
    delete g_Requests[slot];
    g_Pending--;
    g_Closed++;
}

public void Headers(Handle request, bool failed, any sequence, any marker)
{
    int slot = FindSlot(request, sequence, marker);
    if (slot < 0) return;
    if (failed || g_Kind[slot] != 1 || g_Headers[slot]) Failed();
    g_Headers[slot] = true;
}

public void Data(Handle request, bool failed, int offset, int count, any sequence, any marker)
{
    int slot = FindSlot(request, sequence, marker);
    if (slot < 0) return;
    g_StreamCallbacks++;
    char data[6];
    int expected[6] = {'A', 0, 'B', 255, 0, 'Z'};
    bool ok = !failed && g_Kind[slot] == 1 && offset >= 0 && count > 0 && offset + count <= 6;
    if (ok) ok = SteamWorks_GetHTTPStreamingResponseBodyData(request, offset, data, count);
    if (!ok) { Failed(); return; }
    for (int i; i < count; i++)
    {
        int bit = 1 << (offset + i);
        if ((data[i] & 255) != expected[offset + i] || (g_StreamMask[slot] & bit) != 0) Failed();
        g_StreamMask[slot] |= bit;
    }
    g_StreamBytes += count;
}

public void Completed(Handle request, bool failed, bool success, EHTTPStatusCode status, any sequence, any marker)
{
    int slot = FindSlot(request, sequence, marker);
    if (slot < 0) return;
    g_Completed++;
    int kind = g_Kind[slot];
    bool ok = !failed && success && view_as<int>(status) == (kind == 2 ? 503 : 200);
    if (kind == 1) ok = ok && g_Headers[slot] && g_StreamMask[slot] == 63;
    else
    {
        char body[32];
        int size = -1, written = -1;
        bool truncated = true;
        ok = ok && SteamWorks_GetHTTPResponseBodySize(request, size)
            && SteamWorks_GetHTTPResponseBodyString(request, body, sizeof(body), written, truncated)
            && written == size && !truncated;
        if (kind == 0) ok = ok && size == 16 && StrEqual(body, "steamworks-local");
        if (kind == 2) ok = ok && size == 13 && StrEqual(body, "fixture-error");
        if (kind == 3) ok = ok && size == 7 && StrEqual(body, "delayed");
    }
    if (!ok) Failed();
    float latency = (GetEngineTime() - g_SentAt[slot]) * 1000.0;
    g_KindCompleted[kind]++;
    g_LatencySum[kind] += latency;
    if (latency > g_LatencyMax[kind]) g_LatencyMax[kind] = latency;
    int bin = latency <= 50.0 ? 0 : latency <= 100.0 ? 1 : latency <= 250.0 ? 2 : latency <= 1000.0 ? 3 : latency <= 5000.0 ? 4 : 5;
    g_LatencyBins[kind][bin]++;
    CloseSlot(slot);
}
