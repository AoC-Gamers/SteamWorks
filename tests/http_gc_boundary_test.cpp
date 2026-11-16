#include "http_gc_test_host.h"
#include <initializer_list>
int checks=0;
void Check(bool ok,const char* why) { ++checks; if(!ok) { std::fprintf(stderr,"FAIL: %s\n",why); std::exit(1); } }
int main() {
 SteamWorksHTTP manager;g_SteamWorks.pSWHTTP=&manager;
 SteamWorksHTTPRequest request; request.request=17; requestObject=&request;
 IPluginContext ctx; cell_t params[6]={5,1,1,4,8,9};
 for(Native native : {sm_GetHTTPResponseHeaderSize,sm_GetHTTPResponseBodySize,sm_GetHTTPDownloadProgressPct,sm_GetHTTPRequestWasTimedOut}) {
  http.succeeds=false; ctx.storage[1]=0x12345678;
  Check(native(&ctx,params)==0 && ctx.storage[1]==0x12345678,"failed HTTP query leaves output unchanged even if SDK writes");
  http.succeeds=true; ctx.invalidOutput=true; int calls=http.calls;
  Check(native(&ctx,params)==0 && http.calls==calls,"invalid HTTP output never reaches SDK");
  ctx.invalidOutput=false;
  Check(native(&ctx,params)==1,"HTTP output success");
  Check(ctx.storage[0]==0x11223344 && ctx.storage[2]==0x55667788,"HTTP writes one output cell");
 }
 http.timeout=false;ctx.storage[1]=0x12345678;
 Check(sm_GetHTTPRequestWasTimedOut(&ctx,params)==1 && ctx.storage[1]==0,"false writes whole cell");
 http.timeout=true;
 Check(sm_GetHTTPRequestWasTimedOut(&ctx,params)==1 && ctx.storage[1]==1,"true writes whole cell");
 for(Native native : {sm_GetHTTPResponseHeaderSize,sm_GetHTTPResponseBodySize}) {
  http.size=0x80000000;ctx.storage[1]=0x12345678;
  Check(native(&ctx,params)==0 && ctx.storage[1]==0x12345678,"unrepresentable HTTP size rejected without write");
 }
 http.size=4;
 struct LengthCase { Native native;int position; };
 for(auto test : {LengthCase{sm_SetHTTPRequestNetworkActivityTimeout,2},{sm_SetHTTPRequestAbsoluteTimeoutMS,2},{sm_GetHTTPResponseHeaderValue,4},{sm_GetHTTPResponseBodyData,3},{sm_SetHTTPRequestRawPostBody,4},{sm_GetHTTPStreamingResponseBodyData,4}}) {
  cell_t old=params[test.position]; params[test.position]=-1; int calls=http.calls;
  Check(test.native(&ctx,params)==0 && http.calls==calls,"negative HTTP length/timeout rejected before SDK");
  params[test.position]=0;
  Check(test.native(&ctx,params)==1 && http.lastLength==0,"zero HTTP length/timeout retains contract");
  params[test.position]=old;
 }
 params[2]=-1;int calls=http.calls;
 Check(sm_GetHTTPStreamingResponseBodyData(&ctx,params)==0 && http.calls==calls,"negative streaming offset rejected"); params[2]=1;
 ctx.invalidString=true;
 for(Native native : {sm_CreateHTTPRequest,sm_SetHTTPRequestHeaderValue,sm_SetHTTPRequestGetOrPostParameter,sm_GetHTTPResponseHeaderSize,sm_GetHTTPResponseHeaderValue,sm_GetHTTPResponseBodyData,sm_SetHTTPRequestRawPostBody,sm_SetHTTPRequestRawPostBodyFromFile,sm_WriteHTTPResponseBodyToFile,sm_SetHTTPRequestUserAgentInfo}) {
  calls=http.calls;
  Check(native(&ctx,params)==0 && http.calls==calls,"invalid HTTP string rejected before SDK/file access");
 }
 ctx.invalidString=false;
 for(cell_t invalid : {-1,0,99}) { params[1]=invalid;calls=http.calls;Check(sm_CreateHTTPRequest(&ctx,params)==0 && http.calls==calls,"unknown HTTP method rejected"); }
 params[1]=k_EHTTPMethodGET;
 // Use only 4 cells: the arity check must precede any read of params[4/5].
 cell_t shortParams[4]={3,1,1,4};calls=http.calls;
 Check(sm_GetHTTPResponseBodyString(&ctx,shortParams)==0 && http.calls==calls,"obsolete three-argument text call fails before query");
 http.size=0x7fffffff;params[2]=1;params[3]=0;params[4]=0;calls=http.calls;
 Check(sm_GetHTTPResponseBodyCallback(&ctx,params)==0 && http.calls==calls+1,"oversized callback rejected before body allocation/read");
 http.size=4;
 IChangeableForward completed;request.pCompletedForward=&completed;
 Check(sm_SendHTTPRequest(&ctx,params)==1 && request.CompletedCallResult.call==UINT64_C(0xfedcba9876543210),"async call keeps all 64 bits through callback binding");
 request.pCompletedForward=nullptr;
 params[2]=1;params[3]=steamworks::pawn::Cell(0x80000000);params[4]=-1;
 Check(sm_SetHTTPRequestContextValue(&ctx,params)==1 && http.context==UINT64_C(0x0000000180000000),"native context preserves high and low bits");
 IChangeableForward forward;PushContextValue(&forward,UINT64_C(0x80000000ffffffff));
 Check(forward.cells.size()==2 && steamworks::pawn::Bits(forward.cells[0])==0x80000000 && steamworks::pawn::Bits(forward.cells[1])==0xffffffff,"callback context preserves bits");
 params[1]=steamworks::pawn::Cell(0x80000001);params[2]=1;params[3]=-1;int gcCalls=gc.calls;
 Check(sm_GCSendMessage(&ctx,params)==0 && gc.calls==gcCalls,"negative GC size refused");
 params[3]=0;
 Check(sm_GCSendMessage(&ctx,params)==k_EGCResultOK && gc.message==0x80000001 && gc.size==0,"GC type retains protobuf bit and zero length");
 ctx.invalidString=true;gcCalls=gc.calls;
 Check(sm_GCSendMessage(&ctx,params)==0 && gc.calls==gcCalls,"invalid GC data refused");ctx.invalidString=false;
 gameServer.available=false;ctx.invalidOutput=true;ctx.invalidString=true;
 calls=http.calls;int errors=ctx.errors;
 Check(sm_GetHTTPResponseBodySize(&ctx,params)==0 && ctx.errors==errors && http.calls==calls,"HTTP absent capability precedes invalid output");
 Check(sm_GCSendMessage(&ctx,params)==k_EGCResultNotLoggedOn && ctx.errors==errors,"GC absent capability retains public enum");
 gameServer.available=true;ctx.invalidOutput=false;ctx.invalidString=false;handleSystem.invalid=true;
 errors=ctx.errors;calls=http.calls;
 Check(sm_GetHTTPResponseBodySize(&ctx,params)==0 && ctx.errors==errors+1 && http.calls==calls,"bad SourceMod handle never reaches HTTP operation");
 std::printf("PASS: %d HTTP/GC boundary checks (production natives, real SDK interfaces, simulated services)\n",checks);
}
