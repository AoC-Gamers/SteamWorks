#pragma semicolon 1
#pragma newdecls required
#include <sourcemod>
#include <steamworks>

native bool SWForwardFixture_IsActive(int kind);
native int SWForwardFixture_Previous();
native int SWForwardFixture_Observe(int receiver, int kind, bool parametersOk);

public Plugin myinfo = {name = "SteamWorks Forward Receiver", author = "Lechuga", version = "1.0.0"};
int g_Receiver;

public void OnPluginStart()
{
    char filename[PLATFORM_MAX_PATH];
    GetPluginFilename(null, filename, sizeof(filename));
    g_Receiver = filename[strlen(filename) - 5] - '0';
    if (g_Receiver < 1 || g_Receiver > 3) SetFailState("Receiver filename must end in 1.smx, 2.smx or 3.smx");
}

bool Binary(const char[] bytes)
{
    return (bytes[0] & 255) == 0xa5 && bytes[1] == 0 && (bytes[2] & 255) == 0x80
        && bytes[3] == 0x7f && bytes[4] == 0x42 && bytes[5] == 0 && (bytes[6] & 255) == 0xfe;
}
public void SW_OnValidateClient(int ownerauthid, int authid)
{
    if (SWForwardFixture_IsActive(0)) SWForwardFixture_Observe(g_Receiver, 0, ownerauthid == view_as<int>(0xf1234567) && authid == 0x12345678);
}
public void SteamWorks_OnValidateClient(int ownerauthid, int authid)
{
    if (SWForwardFixture_IsActive(1)) SWForwardFixture_Observe(g_Receiver, 1, ownerauthid == view_as<int>(0xf1234567) && authid == 0x12345678);
}
public void SteamWorks_SteamServersConnected()
{
    if (SWForwardFixture_IsActive(2)) SWForwardFixture_Observe(g_Receiver, 2, true);
}
public void SteamWorks_SteamServersConnectFailure(EResult result)
{
    if (SWForwardFixture_IsActive(3)) SWForwardFixture_Observe(g_Receiver, 3, view_as<int>(result) == 17);
}
public void SteamWorks_SteamServersDisconnected(EResult result)
{
    if (SWForwardFixture_IsActive(4)) SWForwardFixture_Observe(g_Receiver, 4, view_as<int>(result) == 17);
}
public void SteamWorks_OnClientGroupStatus(int authid, int groupid, bool isMember, bool isOfficer)
{
    if (SWForwardFixture_IsActive(5)) SWForwardFixture_Observe(g_Receiver, 5, authid == view_as<int>(0xf1234567) && groupid == 0x12345678 && isMember && !isOfficer);
}
public Action SteamWorks_RestartRequested()
{
    if (!SWForwardFixture_IsActive(6)) return Plugin_Continue;
    return view_as<Action>(SWForwardFixture_Observe(g_Receiver, 6, true));
}
public void SteamWorks_TokenRequested(char[] token, int maxlen)
{
    if (!SWForwardFixture_IsActive(7)) return;
    char expected[32];
    int previous = SWForwardFixture_Previous();
    if (previous == 0) strcopy(expected, sizeof(expected), "synthetic-seed");
    else Format(expected, sizeof(expected), "synthetic-%d", previous);
    SWForwardFixture_Observe(g_Receiver, 7, maxlen == sizeof(expected) && StrEqual(token, expected));
    Format(token, maxlen, "synthetic-%d", g_Receiver);
}
// Existing internal forward: it is intentionally absent from steamworks.inc.
public Action SteamWorks_BeginAuthSession(char[] ticket, int length, int authid)
{
    if (!SWForwardFixture_IsActive(8)) return Plugin_Continue;
    int result = SWForwardFixture_Observe(g_Receiver, 8, length == 7 && authid == view_as<int>(0xf1234567) && Binary(ticket) && ticket[7] == 0);
    ticket[0] = 'X'; // No copyback: next consumer must receive the original fixture.
    return view_as<Action>(result);
}
public EGCResults SteamWorks_GCSendMessage(int messageType, const char[] data, int dataLength)
{
    if (!SWForwardFixture_IsActive(9)) return k_EGCResultOK;
    return view_as<EGCResults>(SWForwardFixture_Observe(g_Receiver, 9, messageType == view_as<int>(0x80000042) && dataLength == 7 && Binary(data)));
}
public void SteamWorks_GCMsgAvailable(int dataLength)
{
    if (SWForwardFixture_IsActive(10)) SWForwardFixture_Observe(g_Receiver, 10, dataLength == 7);
}
public EGCResults SteamWorks_GCRetrieveMessage(int messageType, const char[] destination, int destinationLength, int messageSize)
{
    if (!SWForwardFixture_IsActive(11)) return k_EGCResultOK;
    return view_as<EGCResults>(SWForwardFixture_Observe(g_Receiver, 11, messageType == view_as<int>(0x80000042) && destinationLength == 7 && messageSize == 5 && Binary(destination)));
}
