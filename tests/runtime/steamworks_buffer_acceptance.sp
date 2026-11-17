#pragma semicolon 1
#pragma newdecls required
#include <sourcemod>
#include <sdktools>
#include <steamworks>
native int SWBufferFixture_Retrieve(int pointer, int capacity);
native int SWBufferFixture_Local(int pointer,int hooks,int attach,int scenario);
public Plugin myinfo = {name="SteamWorks Buffer Acceptance",author="Lechuga",version="1.0.0"};
Handle g_Getter, g_Request;
int g_Checks, g_Failures, g_Callbacks, g_Return, g_Capacity, g_Received, g_Round;
bool g_GCActive;
int g_LocalScenario=-1;
void Check(bool ok, const char[] name) {
    g_Checks++;
    if (!ok) g_Failures++;
    PrintToServer("[SW-BUFFER] check=%s pass=%d",name,ok);
}
public void OnPluginStart() {
    RegAdminCmd("sm_sw_buffers_gc", CheckGC, ADMFLAG_ROOT);
    RegAdminCmd("sm_sw_buffers_http", CheckHTTP, ADMFLAG_ROOT);
    RegAdminCmd("sm_sw_buffers_gc_local", CheckLocalGC, ADMFLAG_ROOT);
    PrintToServer("[SW-BUFFER] ready");
}
public void OnMapEnd() { delete g_Request; }
public void OnPluginEnd() { delete g_Request; delete g_Getter; }
int AddressArgument(int n) {
    char text[16]; GetCmdArg(n,text,sizeof(text)); return StringToInt(text,16);
}
public Action CheckGC(int client,int args) {
    if(client!=0 || args!=2) return Plugin_Handled;
    int owner=LoadFromAddress(view_as<Address>(AddressArgument(1)),NumberType_Int32);
    Check(owner!=0,"live_candidate_resolver");
    delete g_Getter; StartPrepSDKCall(SDKCall_Raw);
    PrepSDKCall_SetAddress(view_as<Address>(AddressArgument(2)));
    PrepSDKCall_SetReturnInfo(SDKType_PlainOldData,SDKPass_Plain);
    g_Getter=EndPrepSDKCall(); Check(g_Getter!=null,"typed_getter_prepared");
    if(owner==0 || g_Getter==null) return Plugin_Handled;
    int pointer=SDKCall(g_Getter,owner); Check(pointer!=0,"GC_available");
    if(pointer==0) return Plugin_Handled;
    int capacities[]={0,1,3,16,4096}; int results[]={0,-1,2,4};
    for(int c=0;c<sizeof(capacities);c++) {
        for(int r=0;r<sizeof(results);r++) {
            g_Capacity=capacities[c]; g_Return=results[r]; g_Received=0; g_GCActive=true;
            int result=SWBufferFixture_Retrieve(pointer,g_Capacity); g_GCActive=false;
            if(result==-100) { PrintToServer("[SW-BUFFER] GC pending_real_queue_not_consumed=1"); return Plugin_Handled; }
            // ET_Event aggregates negative consumer returns to zero before the hook.
            int expected=g_Return<=0 ? view_as<int>(k_EGCResultNoMessage) : g_Return;
            Check(result==expected,"GC_original_result_or_legacy_override");
            Check(g_Received==1,"GC_exactly_one_real_hook_forward");
        }
    }
    PrintToServer("[SW-BUFFER] GC summary checks=%d failures=%d",g_Checks,g_Failures);
    return Plugin_Handled;
}
public Action CheckLocalGC(int client,int args) {
    if(client!=0 || args!=4 || g_Getter==null) return Plugin_Handled;
    int owner=LoadFromAddress(view_as<Address>(AddressArgument(1)),NumberType_Int32);
    int hooks=LoadFromAddress(view_as<Address>(AddressArgument(3)),NumberType_Int32);
    if(owner==0 || hooks==0) return Plugin_Handled;
    int pointer=SDKCall(g_Getter,owner);
    for(int scenario=0;scenario<5;scenario++) {
        g_LocalScenario=scenario; g_GCActive=true; g_Received=0; g_Return=0;
        int result=SWBufferFixture_Local(pointer,hooks,AddressArgument(4),scenario);
        g_GCActive=false; g_LocalScenario=-1;
        int expected=scenario<3 ? 0 : (scenario==3 ? 1 : 2);
        Check(result==expected,"GC_local_result_outputs_and_target_restored");
        Check(g_Received==1,"GC_local_one_candidate_hook_and_VM_callback");
    }
    PrintToServer("[SW-BUFFER] GC_local summary checks=%d failures=%d",g_Checks,g_Failures);
    return Plugin_Handled;
}
public EGCResults SteamWorks_GCRetrieveMessage(int messageType,const char[] data,int capacity,int size) {
    if(!g_GCActive) return k_EGCResultOK;
    g_Received++;
    if(g_LocalScenario>=0) {
        int cap=g_LocalScenario==0 ? 4096 : (g_LocalScenario==2 ? 0 : 16);
        int valid=g_LocalScenario==0 ? 3 : (g_LocalScenario==1 ? 16 : 0);
        bool bytes=capacity==cap && size==valid && messageType==(g_LocalScenario<3 ? view_as<int>(0x80000042) : 0);
        for(int i=0;i<capacity;i++) {
            int expected=0;
            if(i<valid) expected=g_LocalScenario==1 ? 0x80+i : (i==0 ? 0 : (i==1 ? 0x80 : 0xff));
            if((data[i]&255)!=expected) bytes=false;
        }
        Check(bytes,"GC_local_binary_snapshot_valid_prefix_and_zero_tail");
    } else {
        bool zero=messageType==0 && size==0 && capacity==g_Capacity;
        for(int i=0;i<capacity;i++) if(data[i]!=0) zero=false;
        Check(zero,"GC_error_metadata_and_entire_snapshot_zero");
    }
    return view_as<EGCResults>(g_Return);
}
public Action CheckHTTP(int client,int args) {
    if(client!=0 || g_Request!=null) return Plugin_Handled;
    g_Round=0; StartHTTP(); return Plugin_Handled;
}
void StartHTTP() {
    char url[96]; Format(url,sizeof(url),"http://127.0.0.1:27880/%s",g_Round==1 ? "empty" : (g_Round==2 ? "error" : "text"));
    g_Request=SteamWorks_CreateHTTPRequest(k_EHTTPMethodGET,url);
    Check(g_Request!=null,"HTTP_request_created");
    if(g_Request==null) return;
    // Reading before send must fail without publishing outputs or bytes.
    char pending[8]="ZZZZZZZ"; int written=-123; bool truncated=true;
    bool early=SteamWorks_GetHTTPResponseBodyString(g_Request,pending,sizeof(pending),written,truncated);
    Check(!early && StrEqual(pending,"ZZZZZZZ") && written==-123 && truncated,"HTTP_pending_outputs_unchanged");
    Check(SteamWorks_SetHTTPRequestContextValue(g_Request,21,0x12345678),"HTTP_context_configured");
    Check(SteamWorks_SetHTTPCallbacks(g_Request,Completed),"HTTP_callback_configured");
    Check(SteamWorks_SendHTTPRequest(g_Request),"HTTP_sent");
}
public void Completed(Handle request,bool failed,bool success,EHTTPStatusCode status,any high,any low) {
    g_Callbacks++;
    Check(request==g_Request && !failed && success && high==21 && low==0x12345678,"HTTP_completion_and_context");
    Check(view_as<int>(status)==(g_Round==2 ? 503 : 200),"HTTP_status");
    int size=-1; int expectedSize=g_Round==1 ? 0 : (g_Round==2 ? 13 : 16);
    Check(SteamWorks_GetHTTPResponseBodySize(request,size) && size==expectedSize,"HTTP_real_body_size");
    char expected[32]; strcopy(expected,sizeof(expected),g_Round==2 ? "fixture-error" : "steamworks-local");
    int capacities[]={1,2,4,16,17,32};
    for(int c=0;c<sizeof(capacities);c++) {
        char body[40]; for(int i=0;i<sizeof(body);i++) body[i]='Z';
        int written=-123; bool truncated=false;
        bool read=SteamWorks_GetHTTPResponseBodyString(request,body,capacities[c],written,truncated);
        int count=expectedSize<capacities[c] ? expectedSize : capacities[c]-1;
        bool bytes=body[count]==0;
        for(int i=0;i<count;i++) if(body[i]!=expected[i]) bytes=false;
        for(int i=count+1;i<sizeof(body);i++) if(body[i]!='Z') bytes=false;
        Check(read && written==count && truncated==(expectedSize>=capacities[c]) && bytes,"HTTP_full_read_then_bounded_prefix_NUL_and_canaries");
    }
    delete g_Request;
    Check(g_Request==null,"HTTP_request_closed_in_callback");
    CreateTimer(0.1,NextHTTP,_,TIMER_FLAG_NO_MAPCHANGE);
}
public Action NextHTTP(Handle timer) {
    g_Round++;
    if(g_Round<3) StartHTTP();
    else PrintToServer("[SW-BUFFER] HTTP summary callbacks=%d checks=%d failures=%d",g_Callbacks,g_Checks,g_Failures);
    return Plugin_Stop;
}
