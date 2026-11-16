#include <new>
#include "http_gc_test_host.h"
#include <array>

bool failNextBodyAllocation=false;
int bodyAllocations=0;
void* operator new[](std::size_t size,const std::nothrow_t&) noexcept {
 ++bodyAllocations;
 if(failNextBodyAllocation){failNextBodyAllocation=false;return nullptr;}
 return ::operator new[](size);
}
int checks=0;
void Check(bool ok,const char* label){++checks;if(!ok){std::fprintf(stderr,"FAIL: %s\n",label);std::exit(1);}}
int main(){
 SteamWorksHTTP manager;g_SteamWorks.pSWHTTP=&manager;
 SteamWorksHTTPRequest request;request.request=17;requestObject=&request;
 IPluginContext ctx;std::array<unsigned char,32> bytes;
 struct Output {cell_t before=0x11223344,value=0x12345678,after=0x55667788;} written,truncated;
 ctx.strings[101]=reinterpret_cast<char*>(bytes.data()+4);ctx.outputs[201]=&written.value;ctx.outputs[202]=&truncated.value;
 cell_t p[6]={5,1,101,4,201,202};
 auto reset=[&]{bytes.fill(0xa5);written.value=0x12345678;truncated.value=0x12345678;ctx.invalidOutput=false;ctx.invalidString=false;http.succeeds=true;http.sizeSucceeds=true;http.bodySucceeds=true;http.body={'0','1','2','3','4','5','6','7','8','9'};http.size=10;};
 auto intact=[&]{Check(written.before==0x11223344 && written.after==0x55667788 && truncated.before==0x11223344 && truncated.after==0x55667788,"HTTP output cells keep canaries");};
 reset();int reads=http.bodyReads;
 Check(sm_GetHTTPResponseBodyString(&ctx,p)==1,"strict HTTP truncation reads full provider body");
 Check(http.lastLength==10 && http.bodyReads==reads+1,"HTTP SDK receives full body size rather than destination prefix");
 Check(std::memcmp(bytes.data()+4,"012\0",4)==0 && written.value==3 && truncated.value==1,"HTTP truncation copies prefix, NUL and exact result cells");
 Check(bytes[3]==0xa5 && bytes[8]==0xa5,"HTTP destination truncation canaries intact");intact();
 for(cell_t capacity:{11,16}){
  reset();p[3]=capacity;Check(sm_GetHTTPResponseBodyString(&ctx,p)==1,"HTTP complete body and exact-fit capacity succeed");
  Check(std::memcmp(bytes.data()+4,"0123456789\0",11)==0 && written.value==10 && truncated.value==0,"HTTP full body count/NUL correct");
  Check(bytes[3]==0xa5 && bytes[15]==0xa5,"HTTP full body never writes beyond NUL");intact();
 }
 reset();p[3]=1;reads=http.bodyReads;
 Check(sm_GetHTTPResponseBodyString(&ctx,p)==1 && bytes[4]==0 && bytes[5]==0xa5 && written.value==0 && truncated.value==1,"HTTP one-byte destination is empty terminated/truncated");
 Check(http.lastLength==10 && http.bodyReads==reads+1,"HTTP one-byte destination still uses valid SDK full read");intact();
 reset();http.size=0;http.body.clear();p[3]=4;reads=http.bodyReads;
 Check(sm_GetHTTPResponseBodyString(&ctx,p)==1 && bytes[4]==0 && written.value==0 && truncated.value==0 && http.bodyReads==reads,"HTTP empty body succeeds without non-null SDK allocation/read");intact();
 for(int failure:{0,1}){
  reset();p[3]=16;http.sizeSucceeds=failure!=0;http.bodySucceeds=failure!=1;
  const auto before=bytes;
  Check(sm_GetHTTPResponseBodyString(&ctx,p)==0 && bytes==before,"HTTP failed query/read never publishes partial destination");
  Check(written.value==0x12345678 && truncated.value==0x12345678,"HTTP failed query/read preserves output sentinels");intact();
 }
 reset();failNextBodyAllocation=true;reads=http.bodyReads;
 Check(sm_GetHTTPResponseBodyString(&ctx,p)==0 && !failNextBodyAllocation && http.bodyReads==reads && bytes[4]==0xa5 && written.value==0x12345678 && truncated.value==0x12345678,"HTTP controlled allocation failure does not read SDK body or publish outputs");
 for(uint32 size:{32u*1024u*1024u,static_cast<uint32>(INT32_MAX)}){
  reset();http.size=size;failNextBodyAllocation=true;reads=http.bodyReads;int beforeErrors=ctx.errors;
  Check(sm_GetHTTPResponseBodyString(&ctx,p)==0 && !failNextBodyAllocation && http.bodyReads==reads && ctx.errors==beforeErrors,"HTTP representable large bodies use controlled allocation without silent policy cap");
 }
 reset();http.size=0x80000000;reads=http.bodyReads;int allocations=bodyAllocations,errors=ctx.errors;
 Check(sm_GetHTTPResponseBodyString(&ctx,p)==0 && http.bodyReads==reads && bodyAllocations==allocations && ctx.errors==errors+1 && bytes[4]==0xa5,"HTTP unrepresentable full body is rejected before allocation/read");
 for(cell_t length:{0,-1}){reset();p[3]=length;int calls=http.calls;Check(sm_GetHTTPResponseBodyString(&ctx,p)==0 && http.calls==calls && bytes[4]==0xa5,"HTTP nonpositive length rejected before SDK query");}
 p[3]=4;reset();ctx.invalidOutput=true;int calls=http.calls;Check(sm_GetHTTPResponseBodyString(&ctx,p)==0 && http.calls==calls,"HTTP invalid result address never reaches SDK");
 reset();ctx.invalidString=true;calls=http.calls;Check(sm_GetHTTPResponseBodyString(&ctx,p)==0 && http.calls==calls,"HTTP invalid destination address never reaches SDK");
 reset();cell_t old[4]={3,1,101,4};calls=http.calls;Check(sm_GetHTTPResponseBodyString(&ctx,old)==0 && http.calls==calls,"HTTP obsolete arity fails before absent output argument access");
 std::printf("PASS: %d strict HTTP text response checks (production native, exact-size SDK double)\n",checks);
}
