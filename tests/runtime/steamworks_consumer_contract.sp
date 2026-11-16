#pragma semicolon 1
#pragma newdecls required
#include <sourcemod>
#include <steamworks>
public Plugin myinfo={name="SteamWorks Consumer Contract",author="Lechuga",version="1.0.0"};
int g_Checks,g_Failures,g_UserId;
Handle g_Timer;
float g_Deadline;
void Check(bool ok,const char[] name) {g_Checks++;if(!ok)g_Failures++;PrintToServer("[SW-ABI] check=%s pass=%d",name,ok);}
public void OnPluginStart() {
    char filename[PLATFORM_MAX_PATH],command[64];GetPluginFilename(null,filename,sizeof(filename));
    bool legacy=StrContains(filename,"legacy")!=-1;
    strcopy(command,sizeof(command),legacy?"sm_sw_contract_legacy":"sm_sw_contract_current");RegAdminCmd(command,RunContract,ADMFLAG_ROOT);
    PrintToServer("[SW-ABI] ready legacy_include=%d",legacy);
}
public void OnPluginEnd(){delete g_Timer;}
public void OnMapEnd(){delete g_Timer;}
public Action RunContract(int client,int args) {
    if(client!=0)return Plugin_Handled;
    int target;
    for(int i=1;i<=MaxClients;i++)if(IsClientInGame(i)&&!IsFakeClient(i)&&IsClientAuthorized(i)){target=i;break;}
    if(target==0){PrintToServer("[SW-ABI] awaiting_human=1");return Plugin_Handled;}
    char text[64],validated[64];int count=SteamWorks_GetClientSteamID(target,text,sizeof(text));
    Check(GetClientAuthId(target,AuthId_SteamID64,validated,sizeof(validated),true)&&StrEqual(text,validated)&&count==strlen(text)+1,"H09_full_count_includes_NUL");
    char small[3];Check(SteamWorks_GetClientSteamID(target,small,sizeof(small))==3&&small[2]==0&&small[0]==text[0]&&small[1]==text[1],"H09_truncation_count_NUL");
    char one[2];one[0]='Z';one[1]='Y';Check(SteamWorks_GetClientSteamID(target,one,1)==1&&one[0]==0&&one[1]=='Y',"H09_minimum_capacity");
    one[0]='Z';Check(SteamWorks_GetClientSteamID(target,one,0)==0&&one[0]=='Z'&&one[1]=='Y',"H09_zero_capacity");
    int account=GetSteamAccountID(target,true);
    Check(!SteamWorks_RequestStats(target,1)&&!SteamWorks_RequestStatsAuthID(account,1),"H08_universe_one_is_not_AppID");
    Check(!SteamWorks_RequestStats(target,0)&&!SteamWorks_RequestStatsAuthID(account,0),"H08_zero_AppID_refused");
    Check(SteamWorks_RequestStats(target,550),"H08_client_AppID550_request_accepted");
    Check(SteamWorks_RequestStatsAuthID(account,550),"H08_AccountID_AppID550_request_accepted");
    g_UserId=GetClientUserId(target);g_Deadline=GetEngineTime()+20.0;delete g_Timer;g_Timer=CreateTimer(3.0,ReadStats,_,TIMER_FLAG_NO_MAPCHANGE);
    PrintToServer("[SW-ABI] requested checks=%d failures=%d",g_Checks,g_Failures);return Plugin_Handled;
}
public Action ReadStats(Handle timer) {
    g_Timer=null;int client=GetClientOfUserId(g_UserId);
    Check(client>0&&IsClientInGame(client)&&IsClientAuthorized(client)&&!IsFakeClient(client),"same_authorized_client_for_read");
    if(client>0&&IsClientInGame(client)&&IsClientAuthorized(client)) {
        int account=GetSteamAccountID(client,true),a=0,b=0;
        bool cellOk=SteamWorks_GetStatCell(client,"Stat.GamesPlayed.Total",a);
        bool authOk=SteamWorks_GetStatAuthIDCell(account,"Stat.GamesPlayed.Total",b);
        if((!cellOk||!authOk)&&GetEngineTime()<g_Deadline) {
            g_Timer=CreateTimer(1.0,ReadStats,_,TIMER_FLAG_NO_MAPCHANGE);
            PrintToServer("[SW-ABI] stats_pending=1");return Plugin_Stop;
        }
        Check(cellOk&&authOk&&a==b,"H08_read_routes_same_account_and_stats");
        a=0x12345678;b=0x76543210;
        Check(!SteamWorks_GetStatCell(client,"__steamworks_absent_contract__",a)&&a==0x12345678&&!SteamWorks_GetStatAuthIDCell(account,"__steamworks_absent_contract__",b)&&b==0x76543210,"failed_stats_preserve_outputs");
    }
    PrintToServer("[SW-ABI] complete checks=%d failures=%d",g_Checks,g_Failures);return Plugin_Stop;
}
