// Standalone x86 diagnostic. Does not initialize a gameserver, log on, or call any auth method.
// Only SDK 1.55 / 014 is requested. Its auth slot is inspected, never called.
#include "steam_api.h"
#include "isteamclient.h"
#include "isteamgameserver.h"
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
 void* current=client->GetISteamGameServer(user,pipe,STEAMGAMESERVER_INTERFACE_VERSION);
 if(!current){client->ReleaseUser(pipe,user);client->BReleaseSteamPipe(pipe);return 7;}
 auto** table=*static_cast<void***>(current);
 std::printf("{\"version014_available\":true,\"auth_slot26_present\":%s}\n",table && table[26]?"true":"false");
 client->ReleaseUser(pipe,user);client->BReleaseSteamPipe(pipe);
 client->BShutdownIfAllPipesClosed();
 // Steam client stays mapped until process exit; its worker code must not be unmapped early.
 return 0;
}
