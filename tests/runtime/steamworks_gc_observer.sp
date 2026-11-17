#include <sourcemod>
#include <sdktools>
#include <steamworks>
#pragma semicolon 1
#pragma newdecls required

public Plugin myinfo = {
    name = "SteamWorks GC observer",
    author = "SteamWorks contributors",
    description = "Passive remote GC observation without payload logging",
    version = "1.0"
};

int g_Sends;
int g_Available;
int g_Retrieves;
int g_Nonempty;
int g_Invalid;

public void OnPluginStart()
{
    RegAdminCmd("sm_sw_gc_status", CommandStatus, ADMFLAG_ROOT);
    RegAdminCmd("sm_sw_gc_interface", CommandInterface, ADMFLAG_ROOT);
    PrintToServer("[SW-GC] observer_ready passive=1 payload_logging=0");
}

public Action CommandStatus(int client, int args)
{
    ReplyToCommand(client, "[SW-GC] connected=%d sends=%d available=%d retrieves=%d nonempty=%d invalid=%d",
        SteamWorks_IsConnected(), g_Sends, g_Available, g_Retrieves, g_Nonempty, g_Invalid);
    return Plugin_Handled;
}

// Host must audit the exact ELF/hash, load bias and zero-argument getter before
// passing its address. This command never accepts addresses from a player.
public Action CommandInterface(int client, int args)
{
    if (client != 0 || args != 1)
        return Plugin_Handled;
    char text[16];
    GetCmdArg(1, text, sizeof(text));
    Address getter = view_as<Address>(StringToInt(text, 16));
    if (getter == Address_Null)
        return Plugin_Handled;
    StartPrepSDKCall(SDKCall_Static);
    PrepSDKCall_SetAddress(getter);
    PrepSDKCall_SetReturnInfo(SDKType_PlainOldData, SDKPass_Plain);
    Handle call = EndPrepSDKCall();
    if (call == null)
    {
        PrintToServer("[SW-GC] interface_probe_prepared=0");
        return Plugin_Handled;
    }
    int pointer = SDKCall(call);
    delete call;
    PrintToServer("[SW-GC] interface_probe_prepared=1 interface_available=%d", pointer != 0);
    if (pointer == 0)
        return Plugin_Handled;
    // SDK 1.55 SteamGameCoordinator001: slot 1 is IsMessageAvailable(uint32*).
    // Inspect only: a queued remote message remains owned by its normal reader.
    StartPrepSDKCall(SDKCall_Raw);
    PrepSDKCall_SetVirtual(1);
    PrepSDKCall_SetReturnInfo(SDKType_Bool, SDKPass_Plain);
    PrepSDKCall_AddParameter(SDKType_PlainOldData, SDKPass_Pointer,
        VDECODE_FLAG_BYREF, VENCODE_FLAG_COPYBACK);
    call = EndPrepSDKCall();
    if (call == null)
    {
        PrintToServer("[SW-GC] queue_probe_prepared=0");
        return Plugin_Handled;
    }
    int size = 0;
    bool queued = SDKCall(call, pointer, size);
    delete call;
    PrintToServer("[SW-GC] queue_probe_prepared=1 queued=%d size=%d consumed=0", queued, size);
    return Plugin_Handled;
}

public EGCResults SteamWorks_GCSendMessage(int messageType, const char[] data, int dataLength)
{
    g_Sends++;
    if (g_Sends <= 20)
        PrintToServer("[SW-GC] send type=%d size=%d", messageType, dataLength);
    return k_EGCResultOK;
}

public void SteamWorks_GCMsgAvailable(int dataLength)
{
    g_Available++;
    if (g_Available <= 20)
        PrintToServer("[SW-GC] available size=%d", dataLength);
}

public EGCResults SteamWorks_GCRetrieveMessage(int messageType, const char[] data, int capacity, int size)
{
    g_Retrieves++;
    if (size < 0 || capacity < 0 || size > capacity)
        g_Invalid++;
    else if (size > 0)
        g_Nonempty++;
    if (g_Retrieves <= 20)
        PrintToServer("[SW-GC] retrieve type=%d capacity=%d size=%d", messageType, capacity, size);
    // This forward does not expose the provider's EGCResults. A nonempty snapshot
    // alone cannot establish a successful backend exchange or its provenance.
    return k_EGCResultOK;
}
