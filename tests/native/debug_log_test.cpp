#include "../../extension/core/swdebug.h"
#include "../../extension/core/swdebuglog.h"
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#include <fstream>
#include <string>
#include <cerrno>
int checks=0,errors=0,mkdirCalls=0,writes=0;bool enabled=false;
void Check(bool ok,const char* why) {++checks;if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
bool Make(const char* p) {++mkdirCalls;return ::mkdir(p,0700)==0 || errno==EEXIST;}
void Error() {++errors;}
std::string Read(const std::string& p) {std::ifstream f(p);return std::string(std::istreambuf_iterator<char>(f),{});}
bool TestEnabled(steamworks::debug::Category) {return enabled;}
void TestWrite(steamworks::debug::Category,const char*,const char*,va_list){++writes;}
int main() {
 using namespace steamworks::debug;
 char temp[]="/tmp/steamworks-debug-XXXXXX";char* root=mkdtemp(temp);Check(root!=nullptr,"temporary workspace");
 std::string dir=std::string(root)+"/logs",file=dir+"/debug.log",previous=dir+"/debug.previous.log";
 DebugLog log;log.Initialize(dir.c_str(),Make,Error,512);
 log.Write(Auth,"auth","disabled","must not appear");
 Check(mkdirCalls==0 && access(dir.c_str(),F_OK)!=0,"default off neither formats nor opens a log directory");
 log.SetMask(Auth);log.Write(Hooks,"hooks","excluded","");Check(mkdirCalls==0,"category gate before filesystem");
 log.Write(Auth,"auth","entered","receivers=1");Check(Read(file).find("event=entered receivers=1")!=std::string::npos,"missing directory created and write flushed");
 int dirs=mkdirCalls;log.Write(Auth,"auth","dispatch","result=0");Check(mkdirCalls==dirs,"active file reused without repeated directory work");
 std::string before=Read(file);log.SetMask(0);log.Write(Auth,"auth","disabled","");Check(Read(file)==before,"off closes and stops writes");
 log.SetMask(Auth|Hooks);log.Write(Hooks,"hooks","attach","");Check(Read(file).find(before)==0,"re-enable appends instead of truncating");
 for(unsigned bit : {1u,2u,4u,8u}) {
  log.SetMask(bit);
  for(unsigned category : {1u,2u,4u,8u}) Check(log.Enabled(category)==(category==bit),"each mask selects only its category");
 }
 log.SetMask(Auth);
 for(int i=0;i<50;++i)log.Write(Auth,"auth","repeat","bounded");
 log.Shutdown();Check(Read(file).size()<=512 && !Read(previous).empty() && Read(previous).size()<=512,"repeated rotation keeps two bounded files");
 before=Read(file);log.Write(Auth,"auth","after_shutdown","");Check(Read(file)==before,"shutdown remains off");
 log.Initialize((file+"/invalid").c_str(),Make,Error);log.SetMask(All);
 log.Write(Auth,"auth","failure","");int calls=mkdirCalls;
 for(int i=0;i<10;++i)log.Write(Auth,"auth","repeat_failure","");
 Check(errors==1 && mkdirCalls==calls,"filesystem failure reports once and suppresses retries");
 log.SetMask(0);log.SetMask(All);log.Write(Auth,"auth","still_failed","");Check(errors==1 && mkdirCalls==calls,"mask toggle does not conceal a failed logger");
 log.Initialize(dir.c_str(),Make,Error);log.SetMask(16);Check(!log.Enabled(Auth),"unknown mask bits ignored");
 log.SetMask(Auth);log.Write(Auth,"auth","reinitialized","");Check(Read(file).find("reinitialized")!=std::string::npos,"new extension generation can recover logging");
 GetBackend()={nullptr,nullptr,TestEnabled,TestWrite};enabled=false;
 int formatted=0;Log(Auth,"disabled","%n",&formatted);Check(writes==0 && formatted==0,"backend disabled gate precedes formatting and write");
 enabled=true;Log(Auth,"enabled","receivers=%d",1);Check(writes==1,"enabled backend receives one event");
 GetBackend()={};Log(Auth,"inert","");Check(writes==1,"unconfigured backend is inert");
 std::string defaultDir=std::string(root)+"/default",defaultFile=defaultDir+"/debug.log",defaultPrevious=defaultDir+"/debug.previous.log";
 DebugLog defaultLog;defaultLog.Initialize(defaultDir.c_str(),Make,Error);defaultLog.SetMask(Auth);
 std::string payload(800,'x');
 for(int i=0;i<1500;++i)defaultLog.Write(Auth,"auth","default_limit",payload.c_str());
 defaultLog.Shutdown();
 size_t currentBytes=Read(defaultFile).size(),previousBytes=Read(defaultPrevious).size();
 Check(currentBytes>0 && currentBytes<=1048576 && previousBytes>1048576-1024 && previousBytes<=1048576,
       "default argument preserves the one MiB rotation boundary");
 std::remove(defaultFile.c_str());std::remove(defaultPrevious.c_str());rmdir(defaultDir.c_str());
 log.Shutdown();std::remove(file.c_str());std::remove(previous.c_str());rmdir(dir.c_str());rmdir(root);
 std::printf("PASS: %d debug log checks\n",checks);
}
