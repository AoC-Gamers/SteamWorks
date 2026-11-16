// Disposable offline provider diagnostic, NOT a server or an extension loader.
// Use a network-isolated container, empty HOME and loopback fixture on 27880.
// Does not initialize/log on a game server. All recorded context is synthetic.
#define STEAM_API_NODLL
#include "steam_api.h"
#include <dlfcn.h>
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <cstddef>
static_assert(sizeof(void*)==4,"x86 only");
static_assert(sizeof(HTTPRequestHeadersReceived_t)==12,"x86 headers wire size");
static_assert(sizeof(HTTPRequestDataReceived_t)==20,"x86 data wire size");
static_assert(offsetof(HTTPRequestDataReceived_t,m_ulContextValue)==4,"x86 context offset");
int main(int argc,char**argv) {
 if(argc!=2)return 2;
 auto module=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);if(!module){puts(dlerror());return 2;}
 auto create=reinterpret_cast<void*(*)(const char*,int*)>(dlsym(module,"CreateInterface"));
 auto get=reinterpret_cast<bool(*)(HSteamPipe,CallbackMsg_t*,int*)>(dlsym(module,"Steam_BGetCallback"));
 auto freecb=reinterpret_cast<void(*)(HSteamPipe)>(dlsym(module,"Steam_FreeLastCallback"));
 auto*client=static_cast<ISteamClient*>(create(STEAMCLIENT_INTERFACE_VERSION,nullptr)); if(!client||!get||!freecb)return 3;
 HSteamPipe pipe=0;auto user=client->CreateLocalUser(&pipe,k_EAccountTypeGameServer);printf("local user=%d pipe=%d\n",user!=0,pipe!=0);fflush(stdout);if(!user||!pipe)return 4;
 auto*http=client->GetISteamHTTP(user,pipe,STEAMHTTP_INTERFACE_VERSION);if(!http)return 5;
 auto*utils=client->GetISteamUtils(pipe,STEAMUTILS_INTERFACE_VERSION);
 if(!utils)return 5;
 auto h=http->CreateHTTPRequest(k_EHTTPMethodGET,"http://127.0.0.1:27880/stream");
 bool set=http->SetHTTPRequestContextValue(h,UINT64_C(0x0000000712345678));
 printf("set=%d\n",set);
 SteamAPICall_t call=0;bool sent=http->SendHTTPRequestAndStreamResponse(h,&call);
 printf("send=%d\n",sent);fflush(stdout);
 bool done=false,valid=set&&sent;int headers=0;unsigned bytes=0;
 for(int i=0;i<500&&!done;++i){
  client->RunFrame(); CallbackMsg_t msg={};
  while(get(pipe,&msg,nullptr)){
   if(msg.m_iCallback==2102||msg.m_iCallback==2103){
    printf("callback=%d bytes=%d wire=",msg.m_iCallback,msg.m_cubParam);for(int n=0;n<msg.m_cubParam;++n)printf("%02x",msg.m_pubParam[n]);puts("");
    if(msg.m_iCallback==2102){++headers;valid=valid&&msg.m_cubParam==12;}
    else if(msg.m_cubParam==20){HTTPRequestDataReceived_t data;std::memcpy(&data,msg.m_pubParam,sizeof(data));valid=valid&&data.m_cOffset==bytes;bytes+=data.m_cBytesReceived;}
    else valid=false;
   }
   freecb(pipe);
  }
  bool failed=false;if(utils->IsAPICallCompleted(call,&failed)){
   HTTPRequestCompleted_t c={};bool got=utils->GetAPICallResult(call,&c,sizeof(c),c.k_iCallback,&failed);
   printf("completion got=%d failed=%d context=%016llx\n",got,failed,(unsigned long long)c.m_ulContextValue);done=true;
   valid=valid&&got&&!failed&&c.m_bRequestSuccessful&&c.m_eStatusCode==200&&c.m_ulContextValue==UINT64_C(0x0000000712345678);
  }
  usleep(10000);
 }
 http->ReleaseHTTPRequest(h);client->ReleaseUser(pipe,user);client->BReleaseSteamPipe(pipe);client->BShutdownIfAllPipesClosed();
 return done&&valid&&headers==1&&bytes==6?0:6;
}
