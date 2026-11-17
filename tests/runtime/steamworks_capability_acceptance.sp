#pragma semicolon 1
#pragma newdecls required
#include <sourcemod>
#include <sdktools>
#include <steamworks>
native bool SWCapabilityFixture_Probe(int kind, int pointer);
public Plugin myinfo = {name="SteamWorks Capability Acceptance",author="Lechuga",version="1.0.0"};
Handle g_Getters[9];
int g_Owner;
int g_Checks, g_Failures, g_GCSends;
bool g_BlockGC, g_GCPayload;
void Check(bool ok, const char[] name) {
    g_Checks++;
    if (!ok) g_Failures++;
    PrintToServer("[SW-CAP] check=%s pass=%d",name,ok);
}
public void OnPluginStart() {
    RegAdminCmd("sm_sw_capabilities", InspectCapabilities, ADMFLAG_ROOT);
    PrintToServer("[SW-CAP] ready");
}
public void OnPluginEnd() {
    for (int i=0;i<sizeof(g_Getters);i++) delete g_Getters[i];
}
int AddressArgument(int n) {
    char text[16];GetCmdArg(n,text,sizeof(text));return StringToInt(text,16);
}
public Action InspectCapabilities(int client,int args) {
    if (client!=0) return Plugin_Handled;
    if (args==10) {
        // Host verifies exact ELF/hash, entry ranges, and the audited owner slot.
        g_Owner=LoadFromAddress(view_as<Address>(AddressArgument(1)),NumberType_Int32);
        Check(g_Owner!=0,"live_candidate_resolver");
        for(int i=0;i<sizeof(g_Getters);i++) {
            delete g_Getters[i];StartPrepSDKCall(SDKCall_Raw);
            PrepSDKCall_SetAddress(view_as<Address>(AddressArgument(i+2)));
            PrepSDKCall_SetReturnInfo(SDKType_PlainOldData,SDKPass_Plain);
            g_Getters[i]=EndPrepSDKCall();Check(g_Getters[i]!=null,"getter_call_prepared");
        }
    }
    if(g_Owner==0) return Plugin_Handled;
    char mode[16];GetCmdArg(1,mode,sizeof(mode));
    bool rejected=StrEqual(mode,"rejected");
    int pointers[9];
    for(int kind=0;kind<sizeof(g_Getters);kind++) {
        pointers[kind]=SDKCall(g_Getters[kind],g_Owner);
        int repeated=SDKCall(g_Getters[kind],g_Owner);
        Check(repeated==pointers[kind],"stable_resolver_result");
        bool probed=pointers[kind]!=0 && SWCapabilityFixture_Probe(kind,pointers[kind]);
        PrintToServer("[SW-CAP] interface kind=%d available=%d typed_probe=%d",kind,pointers[kind]!=0,probed);
        if(kind==6 && pointers[kind]==0) Check(!probed,"matchmaking_absent_not_called");
        else if(kind!=7) Check(probed,"provided_interface_minimum_query");
        else if(rejected) Check(pointers[kind]==0,"unknown_GC_version_refused");
        else if(pointers[kind]!=0) Check(probed,"provided_GC_minimum_query");
        // GC availability is independently classified from its backend session.
    }
    Check(SteamWorks_IsLoaded() && SteamWorks_IsConnected(),"public_gameserver_loaded_connected");
    int ip[4];Check(SteamWorks_GetPublicIP(ip),"public_IPv4_query");
    int cell=-123;float real=123.25;
    Check(!SteamWorks_GetStatAuthIDCell(0,"__steamworks_capability_missing__",cell) && cell==-123,"public_stats_failure_preserves_cell");
    Check(!SteamWorks_GetStatAuthIDFloat(0,"__steamworks_capability_missing__",real) && real==123.25,"public_stats_failure_preserves_float");
    if(pointers[7]!=0) {
        char data[4];data[0]=0x41;data[1]=0;data[2]=0x7f;data[3]=0x42;
        int before=g_GCSends;g_BlockGC=true;g_GCPayload=false;
        EGCResults result=SteamWorks_SendMessageToGC(0x7ffffffe,data,sizeof(data));g_BlockGC=false;
        Check(g_GCSends==before+1 && g_GCPayload && result==k_EGCResultInvalidMessage,"public_GC_send_locally_intercepted");
    } else {
        char data[1];
        Check(SteamWorks_SendMessageToGC(0,data,0)==k_EGCResultNotLoggedOn,"public_GC_absent_enum");
    }
    PrintToServer("[SW-CAP] summary checks=%d failures=%d GC_available=%d rejected_mode=%d",g_Checks,g_Failures,pointers[7]!=0,rejected);
    return Plugin_Handled;
}
public EGCResults SteamWorks_GCSendMessage(int messageType,const char[] data,int dataLength) {
    if(!g_BlockGC) return k_EGCResultOK;
    g_GCSends++;
    g_GCPayload=messageType==0x7ffffffe && dataLength==4 && data[0]==0x41 && data[1]==0 && data[2]==0x7f && data[3]==0x42;
    // SourceHook supersedes the provider: the fixture never reaches Steam GC.
    return k_EGCResultInvalidMessage;
}
