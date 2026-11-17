#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
int checks=0,clientPipe=1,serverPipe=1,shutdowns=0;
std::vector<std::string> events;
void Check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
int SteamAPI_GetHSteamPipe(){return clientPipe;}
int SteamGameServer_GetHSteamPipe(){return serverPipe;}
void SteamAPI_Shutdown(){events.push_back("client-shutdown");++shutdowns;clientPipe=0;}
#include "../../extension/core/swclientlifecycle.cpp"
int main(){
 SteamWorksClientLifecycle owner;
 owner.FinalizeStoppedHost();Check(shutdowns==0 && clientPipe==1,"load failure or ordinary hot unload cannot close host session");
 owner.GameServerStopped();owner.FinalizeStoppedHost();Check(shutdowns==0 && clientPipe==1,"observed shutdown with still-live GS pipe refuses client close");
 owner.GameServerStarted();serverPipe=0;owner.FinalizeStoppedHost();Check(shutdowns==0,"successful reinit cancels stale shutdown observation");
 owner.GameServerStopped();owner.FinalizeStoppedHost();Check(shutdowns==1 && clientPipe==0,"observed stopped host teardown closes unmatched application role");
 owner.FinalizeStoppedHost();Check(shutdowns==1,"duplicate unload is inert");
 owner.GameServerStopped();owner.FinalizeStoppedHost();Check(shutdowns==1,"already closed application role is not shut down again");
 clientPipe=2;owner.FinalizeStoppedHost();Check(shutdowns==1,"completed teardown cannot close a replacement role");
 owner.GameServerStarted();serverPipe=1;owner.FinalizeStoppedHost();Check(shutdowns==1 && clientPipe==2,"new live lifetime is preserved");
 owner.GameServerStopped();serverPipe=0;owner.FinalizeStoppedHost();Check(shutdowns==2 && clientPipe==0,"new balanced shutdown can retire the new lifetime");
 Check(events==std::vector<std::string>{"client-shutdown","client-shutdown"},"only actual finalizations invoke SDK shutdown");
 std::printf("PASS: %d client lifecycle checks (production finalizer, simulated SDK role API)\n",checks);
}
