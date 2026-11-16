// Standalone x86 diagnostic. Does not initialize a gameserver, log on, or call any auth method.
// The 014 object is opaque: its table is inspected, never cast to the 015 contract.
#include "steam_api.h"
#include "isteamclient.h"
#include <dlfcn.h>
#include <cstdio>
int main(int argc,char** argv) {
 if(argc!=2)return 2;
 void* lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
 if(!lib){std::fprintf(stderr,"dlopen failed\n");return 3;}
 using Factory=void*(*)(const char*,int*);
 auto factory=reinterpret_cast<Factory>(dlsym(lib,"CreateInterface"));
 if(!factory)return 4;
 auto* client=static_cast<ISteamClient*>(factory("SteamClient020",nullptr));
 if(!client)return 5;
 HSteamPipe pipe=0;HSteamUser user=client->CreateLocalUser(&pipe,k_EAccountTypeGameServer);
 if(!user || !pipe)return 6;
 void* old=client->GetISteamGameServer(user,pipe,"SteamGameServer014");
 void* current=client->GetISteamGameServer(user,pipe,"SteamGameServer015");
 if(!old || !current){client->ReleaseUser(pipe,user);client->BReleaseSteamPipe(pipe);return 7;}
 auto** oldTable=*static_cast<void***>(old);auto** currentTable=*static_cast<void***>(current);
 std::printf("{\"version014_available\":true,\"version015_available\":true,\"same_object\":%s,\"same_vtable\":%s,\"same_slot26_target\":%s}\n",old==current?"true":"false",oldTable==currentTable?"true":"false",oldTable[26]==currentTable[26]?"true":"false");
 client->ReleaseUser(pipe,user);client->BReleaseSteamPipe(pipe);
 client->BShutdownIfAllPipesClosed();
 // Steam client stays mapped until process exit; its worker code must not be unmapped early.
 return 0;
}
