#define _INCLUDE_SOURCEMOD_EXTENSION_BASESDK_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#include "steamclientpublic.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <type_traits>
#include <initializer_list>
#include <limits>
#ifdef STEAMWORKS_TEST_WINDOWS_MAX_MACRO
#define max(left, right) ((left) > (right) ? (left) : (right))
#endif
using cell_t = int32_t;
constexpr int SP_ERROR_NONE = 0;
struct IPluginContext {
 int errors=0; bool invalid=false; cell_t cell=0x12345678; char data[8]="test";
 int LocalToString(cell_t,char** out) { if(invalid) return 42; *out=data; return 0; }
 int LocalToPhysAddr(cell_t,cell_t** out) { if(invalid) return 42; *out=&cell; return 0; }
 cell_t ThrowNativeError(const char*,...) { ++errors; return 0; }
 cell_t ThrowNativeErrorEx(int,const char*,...) { ++errors; return 0; }
};
#include "../extension/core/swpawn.h"
#include "../extension/core/swauthticket.h"
#if defined(STEAMWORKS_TEST_WINDOWS_MAX_MACRO) && !defined(max)
#error Windows macro regression must not disable max
#endif
struct IGamePlayer {
 const char* auth=nullptr; AccountID_t account=0xfedcba98;
 const char* GetAuthString(bool) { return auth; }
 AccountID_t GetSteamAccountID(bool) { return account; }
};
struct SteamWorks {
 CSteamID CreateCommonCSteamID(IGamePlayer*,EUniverse,EAccountType);
 CSteamID CreateCommonCSteamID(steamworks::AccountId,EUniverse,EAccountType);
};
#include "../extension/core/swidentity.cpp"
int checks=0;
void Check(bool ok,const char* why) { ++checks; if(!ok) { std::fprintf(stderr,"FAIL: %s\n",why); std::exit(1); } }
int main() {
 using namespace steamworks;
 static_assert(!std::is_convertible<AppId,AccountId>::value,"AppID must not become identity");
 static_assert(!std::is_convertible<cell_t,AccountId>::value,"Cells require explicit decoding");
 for(uint32 bits : {0u,1u,0x7fffffffu,0x80000000u,0xffffffffu}) {
  Check(pawn::Bits(pawn::Cell(bits))==bits,"all cell bits preserved");
  Check(pawn::Account(pawn::Cell(bits)).value==bits,"account bits preserved");
  Check(pawn::App(pawn::Cell(bits)).value==bits,"application bits preserved");
 }
 Check(PackContext(0x80000000,0xffffffff)==UINT64_C(0x80000000ffffffff),"context high and low sign bits");
 IPluginContext ctx; ByteCount count(99); uint32 size=123;
 Check(!pawn::Size(&ctx,-1,count) && count.value==99 && ctx.errors==1,"negative size refused without mutation");
 Check(pawn::SteamSize(&ctx,0,size) && size==0,"zero allowed per contract");
 Check(!pawn::SteamSize(&ctx,0,size,false),"positive-only contract");
 Check(pawn::SteamSize(&ctx,INT32_MAX,size) && size==INT32_MAX,"maximum cell length");
 size=123;
 if(sizeof(size_t)>4) Check(!ToSteamSize(ByteCount(static_cast<size_t>(UINT32_MAX)+1),size) && size==123,"size_t overflow rejected before narrowing");
 Check(ToSteamSize(ByteCount(UINT32_MAX),size) && size==UINT32_MAX,"maximum Steam length");
 ctx.invalid=true; char* string=nullptr; cell_t* output=nullptr;
 Check(!pawn::String(&ctx,0,string) && string==nullptr,"bad string conversion");
 Check(!pawn::Output(&ctx,0,output) && output==nullptr,"bad output conversion");
 ctx.invalid=false;
 Check(pawn::String(&ctx,0,string) && string==ctx.data,"valid string conversion");
 Check(pawn::Output(&ctx,0,output) && output==&ctx.cell,"valid output conversion");
 Check(!pawn::StoreSize(&ctx,output,0x80000000) && ctx.cell==0x12345678,"oversized output leaves cell untouched");
 Check(pawn::StoreSize(&ctx,output,INT32_MAX) && ctx.cell==INT32_MAX,"representable size output");
 pawn::StoreFloat(output,-3.25f); float number; std::memcpy(&number,output,4);
 Check(number==-3.25f,"float bits stored without pointer alias");
 EUniverse universe; EAccountType type; cell_t params[6]={3,0,0,0,0,0};
 Check(pawn::IdentityOptions(&ctx,params,4,5,universe,type) && universe==k_EUniversePublic && type==k_EAccountTypeIndividual,"omitted identity options retain defaults");
 params[0]=5;
 for(cell_t invalid : {-1,static_cast<cell_t>(k_EUniverseMax),INT32_MAX}) {
  params[4]=invalid; Check(!pawn::IdentityOptions(&ctx,params,4,5,universe,type),"invalid universe rejected");
 }
 params[4]=k_EUniversePublic;
 for(cell_t invalid : {-1,static_cast<cell_t>(k_EAccountTypeMax),INT32_MAX}) {
  params[5]=invalid; Check(!pawn::IdentityOptions(&ctx,params,4,5,universe,type),"invalid account type rejected");
 }
 params[4]=0; params[5]=0;
 Check(pawn::IdentityOptions(&ctx,params,4,5,universe,type),"legacy zero options remain defined");
 SteamWorks service; IGamePlayer player;
 for(const char* auth : {static_cast<const char*>(nullptr),"","short","STEAM_ID_PENDING"}) {
  player.auth=auth; auto identity=service.CreateCommonCSteamID(&player,k_EUniverseBeta,k_EAccountTypeIndividual);
  Check(identity.GetAccountID()==player.account && identity.GetEUniverse()==k_EUniverseBeta,"fallback options decoded before operation");
 }
 for(const char* auth : {"STEAM_1:1:123","[U:1:123]","STEAM_0:1:123","STEAM_999:1:123"}) {
  player.auth=auth; auto identity=service.CreateCommonCSteamID(&player,k_EUniverseBeta,k_EAccountTypeClan);
  Check(identity.GetAccountID()==player.account && identity.GetEUniverse()==k_EUniversePublic && identity.GetEAccountType()==k_EAccountTypeIndividual,"normal identity retains existing resolution");
 }
 Check(service.CreateCommonCSteamID(AccountId(0xffffffff),k_EUniversePublic,k_EAccountTypeIndividual).GetAccountID()==0xffffffff,"typed numeric operation keeps all account bits");
 std::printf("PASS: %d typed boundary/identity checks (real SDK types, simulated Pawn memory)\n",checks);
}
