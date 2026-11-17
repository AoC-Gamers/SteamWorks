#pragma semicolon 1
#pragma newdecls required
#include <sourcemod>
// Exact incompatible three-argument declaration shape from the external include.
native Handle SteamWorks_CreateHTTPRequest(int method,const char[] url);
native bool SteamWorks_GetHTTPResponseBodyString(Handle request,char[] buffer,int length);
public Plugin myinfo={name="SteamWorks Legacy HTTP Consumer",author="Lechuga",version="1.0.0"};
bool g_Returned;
Handle g_Request;
char g_Buffer[16];
public void LegacyRead(){SteamWorks_GetHTTPResponseBodyString(g_Request,g_Buffer,sizeof(g_Buffer));g_Returned=true;}
public void OnPluginStart(){RegAdminCmd("sm_sw_legacy_http_check",CheckLegacy,ADMFLAG_ROOT);}
public Action CheckLegacy(int client,int args) {
    if(client!=0)return Plugin_Handled;
    g_Request=SteamWorks_CreateHTTPRequest(1,"http://127.0.0.1:27880/text");
    for(int i=0;i<sizeof(g_Buffer);i++)g_Buffer[i]='Z';g_Returned=false;
    Call_StartFunction(GetMyHandle(),LegacyRead);int error=Call_Finish();bool unchanged=true;
    for(int i=0;i<sizeof(g_Buffer);i++)if(g_Buffer[i]!='Z')unchanged=false;
    delete g_Request;
    PrintToServer("[SW-ARITY] rejected=%d unchanged=%d returned=%d error=%d",error==SP_ERROR_NATIVE,unchanged,g_Returned,error);
    return Plugin_Handled;
}
