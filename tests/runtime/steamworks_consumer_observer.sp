#pragma semicolon 1
#pragma newdecls required
#include <sourcemod>
#include <steamworks>
#undef REQUIRE_PLUGIN
#include <l4d2_familyshare_core>
#define REQUIRE_PLUGIN

public Plugin myinfo={name="SteamWorks Consumer Observer",author="Lechuga",version="1.0.0"};
enum struct PluginRuntimeState {
    bool lateLoad;
    bool hasFamily;
    void Reset() { this.lateLoad=false;this.hasFamily=false; }
}
PluginRuntimeState g_Runtime;
int g_Checks,g_Failures,g_ConfigBlocks,g_ProfileId,g_ProfileEvents;
void Check(bool ok,const char[] name) {
    g_Checks++;if(!ok)g_Failures++;
    PrintToServer("[SW-CONS] check=%s pass=%d",name,ok);
}
public APLRes AskPluginLoad2(Handle myself,bool late,char[] error,int length) {
    g_Runtime.Reset();g_Runtime.lateLoad=late;return APLRes_Success;
}
public void OnPluginStart() {
    AddCommandListener(BlockConsumerConfig,"exec");
    RegAdminCmd("sm_sw_consumer",InspectConsumer,ADMFLAG_ROOT);
    g_Runtime.hasFamily=LibraryExists(L4D2_FAMILYSHARE_CORE_LIBRARY);
    PrintToServer("[SW-CONS] observer_ready");
}
public void OnAllPluginsLoaded(){g_Runtime.hasFamily=LibraryExists(L4D2_FAMILYSHARE_CORE_LIBRARY);}
public void OnLibraryAdded(const char[] name){if(StrEqual(name,L4D2_FAMILYSHARE_CORE_LIBRARY))g_Runtime.hasFamily=true;}
public void OnLibraryRemoved(const char[] name){if(StrEqual(name,L4D2_FAMILYSHARE_CORE_LIBRARY)){g_Runtime.hasFamily=false;g_ProfileId=0;}}
public Action BlockConsumerConfig(int client,const char[] command,int argc) {
    char path[PLATFORM_MAX_PATH];GetCmdArg(1,path,sizeof(path));
    if(StrEqual(path,"sourcemod/AoC.cfg",false)||StrEqual(path,"sourcemod/l4d2_familyshare_core.cfg",false)) {
        g_ConfigBlocks++;PrintToServer("[SW-CONS] protected_config_exec_blocked=1");return Plugin_Handled;
    }
    return Plugin_Continue;
}
void IntegerSetting(const char[] name,int value) { ConVar c=FindConVar(name);if(c!=null)c.SetInt(value,false,false); }
public Action InspectConsumer(int client,int args) {
    if(client!=0)return Plugin_Handled;
    char mode[32],file[PLATFORM_MAX_PATH];GetCmdArg(1,mode,sizeof(mode));GetCmdArg(2,file,sizeof(file));
    if(StrEqual(mode,"loaded")) {
        Handle plugin=FindPluginByFile(file);Check(plugin!=null&&GetPluginStatus(plugin)==Plugin_Running,"real_consumer_running");
    } else if(StrEqual(mode,"unloaded"))Check(FindPluginByFile(file)==null,"real_consumer_unloaded");
    else if(StrEqual(mode,"aoc")) {
        Handle plugin=FindPluginByFile(file);Check(plugin!=null,"aoc_loaded");if(plugin==null)return Plugin_Handled;
        IntegerSetting("sm_aoc_debug",0);IntegerSetting("sm_aoc_forcelerp0",0);IntegerSetting("sm_aoc_server_type",1);IntegerSetting("sm_aoc_solo_ready_guard",0);
        IntegerSetting("sm_aoc_welcome_chat_basic",0);IntegerSetting("sm_aoc_welcome_console_block",0);IntegerSetting("sm_aoc_welcome_social_block",0);
        ConVar force=FindConVar("sm_aoc_forceconfogl");if(force!=null)force.SetString("",false,false);
        ConVar host=FindConVar("hostip"),net=FindConVar("net_public_adr");char oldHost[96],oldNet[96];host.GetString(oldHost,sizeof(oldHost));net.GetString(oldNet,sizeof(oldNet));
        Function f=GetFunctionByName(plugin,"SteamWorks_SteamServersConnected");Check(f!=INVALID_FUNCTION,"aoc_real_callback_found");
        if(f!=INVALID_FUNCTION) {Call_StartFunction(plugin,f);int error=Call_Finish();Check(error==SP_ERROR_NONE,"aoc_IP_callback_dispatch");}
        int ip[4];char expected[64],actual[96];bool obtained=SteamWorks_GetPublicIP(ip);Format(expected,sizeof(expected),"%d.%d.%d.%d",ip[0],ip[1],ip[2],ip[3]);net.GetString(actual,sizeof(actual));Check(obtained&&StrEqual(actual,expected),"aoc_IP_consumed_without_exporting_address");
        host.SetString(oldHost,false,false);net.SetString(oldNet,false,false);
    } else if(StrEqual(mode,"family")) {
        Check(g_Runtime.hasFamily,"family_library_present");if(!g_Runtime.hasFamily)return Plugin_Handled;
        ConVar key=FindConVar("l4d2_familyshare_steam_api_key");Check(key!=null,"family_key_control_available");if(key!=null)key.SetString("",false,false);
        Check(!L4D2FamilyShareCore_IsClientDetected(0)&&L4D2FamilyShareCore_GetClientBorrowerAccountId(0)==0&&L4D2FamilyShareCore_GetClientOwnerAccountId(0)==0,"family_invalid_client_state");
        Check(L4D2FamilyShareCore_RequestProfile(0)==0,"family_zero_account_refused");
        g_ProfileEvents=0;g_ProfileId=L4D2FamilyShareCore_RequestProfile(123456789);Check(g_ProfileId>0,"family_fallback_queued");
    } else if(StrEqual(mode,"absent"))Check(!g_Runtime.hasFamily,"family_removed_observed");
    PrintToServer("[SW-CONS] summary checks=%d failures=%d config_blocks=%d profile_events=%d",g_Checks,g_Failures,g_ConfigBlocks,g_ProfileEvents);
    return Plugin_Handled;
}
public void L4D2FamilyShareCore_OnProfileResolved(int requestId,int accountId,const char[] steamId64,const char[] personaName,const char[] profileUrl) {
    if(requestId!=g_ProfileId)return;
    g_ProfileEvents++;
    Check(accountId==123456789&&StrEqual(steamId64,"76561198083722517")&&personaName[0]==0&&StrEqual(profileUrl,"https://steamcommunity.com/profiles/76561198083722517"),"family_local_profile_callback_fields");
    Check(g_ProfileEvents==1,"family_profile_exactly_once");g_ProfileId=0;
    PrintToServer("[SW-CONS] profile_complete events=%d failures=%d",g_ProfileEvents,g_Failures);
}
