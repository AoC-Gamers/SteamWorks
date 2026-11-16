#define TEST_REAL_STEAM_CALLBACKS
#include "steam_callbacks_test_host.h"
#include "http_gc_test_host.h"
#include <cstddef>

static_assert(sizeof(void*) == 4, "Streaming regression must run on x86");
static_assert(sizeof(HTTPRequestHeadersReceived_t) == 12, "Headers wire size");
static_assert(sizeof(HTTPRequestDataReceived_t) == 20, "Data wire size");
static_assert(offsetof(HTTPRequestDataReceived_t, m_ulContextValue) == 4, "Context wire offset");
static_assert(offsetof(HTTPRequestDataReceived_t, m_cOffset) == 12, "Offset wire offset");
int checks = 0;
void Check(bool ok, const char* why) {
 ++checks;
 if (!ok) { std::fprintf(stderr, "FAIL: %s\n", why); std::exit(1); }
}
void Word(unsigned char* wire, size_t offset, uint32 value) {
 for (unsigned i = 0; i < 4; ++i) { wire[offset+i] = static_cast<unsigned char>(value >> (8*i)); }
}
// Independent little-endian wire fixture: the real provider retains the low
// word, zeros the high word, and delivers correct offset/count (see report).
void Stream(HTTPRequestHandle id, uint32 low, uint32 offset = 3) {
 unsigned char wire[20] = {};
 Word(wire, 0, id); Word(wire, 4, low); Word(wire, 12, offset); Word(wire, 16, 3);
 HTTPRequestHeadersReceived_t headers;
 HTTPRequestDataReceived_t data;
 std::memcpy(&headers, wire, sizeof(headers)); std::memcpy(&data, wire, sizeof(data));
 DeliverCallback(headers); DeliverCallback(data);
}
SteamWorksHTTPRequest* NewRequest(SteamWorksHTTP& manager, HTTPRequestHandle id, SteamAPICall_t call) {
 auto* r = new SteamWorksHTTPRequest(&http);
 r->request = id;
 r->handle = handlesys->CreateHandle(manager.GetHTTPHandle(), r, nullptr, myself->GetIdentity(), nullptr);
 requestObject = r;
 IPluginContext context;
 cell_t args[] = {5, static_cast<cell_t>(r->handle), 1, 2, 3, 0};
 Check(sm_SetCallbacks(&context, args) == 1, "configure public callbacks");
 SetCallbacks(call, r);
 return r;
}
bool Set(SteamWorksHTTPRequest* r, uint32 high, uint32 low) {
 requestObject = r; IPluginContext context;
 cell_t args[] = {3, static_cast<cell_t>(r->handle), steamworks::pawn::Cell(high), steamworks::pawn::Cell(low)};
 return sm_SetHTTPRequestContextValue(&context, args) == 1;
}
void ExpectStream(SteamWorksHTTPRequest* r, uint32 high, uint32 low, uint32 offset = 3) {
 const auto& h = r->pHeadersReceivedForward->cells;
 const auto& d = r->pDataReceivedForward->cells;
 Check(h.size() == 4 && d.size() == 6, "public streaming arity unchanged");
 Check(h[2] == steamworks::pawn::Cell(high) && h[3] == steamworks::pawn::Cell(low), "headers preserve both accepted context cells");
 Check(d[4] == steamworks::pawn::Cell(high) && d[5] == steamworks::pawn::Cell(low), "data preserve both accepted context cells");
 Check(h[0] == static_cast<cell_t>(r->handle) && d[0] == h[0] && h[1] == 0 && d[1] == 0, "handle and failure arguments unchanged");
 Check(d[2] == steamworks::pawn::Cell(offset) && d[3] == 3, "stream offset and count unchanged");
 r->pHeadersReceivedForward->cells.clear(); r->pDataReceivedForward->cells.clear();
}
int main() {
 SteamWorksHTTP manager; g_SteamWorks.pSWHTTP = &manager;
 auto* a = NewRequest(manager, 17, 91);
 Stream(17, 0); ExpectStream(a, 0, 0); // No setter: zero default.
 http.succeeds = false;
 Check(!Set(a, 7, 0x12345678), "first setter rejection retains default context");
 http.succeeds = true;
 Stream(17, 0); ExpectStream(a, 0, 0);
 const uint32 patterns[][2] = {{7,0x12345678}, {0xffffffff,0x80000000}, {0x80000000,0xffffffff}, {0,0}, {0xffffffff,0}};
 for (const auto& pair : patterns) {
  Check(Set(a, pair[0], pair[1]), "context setter succeeds");
  Check(http.context == steamworks::PackContext(pair[0], pair[1]), "SDK receives all 64 bits");
  Stream(999, pair[1]);
  Check(a->pHeadersReceivedForward->cells.empty() && a->pDataReceivedForward->cells.empty(), "foreign handle ignored");
  Stream(17, pair[1]); ExpectStream(a, pair[0], pair[1]);
 }
 Check(Set(a, 7, 0x12345678), "restore runtime reproduction context");
 http.succeeds = false;
 Check(!Set(a, 99, 55), "SDK rejects replacement context");
 http.succeeds = true;
 Stream(17, 0x12345678); ExpectStream(a, 7, 0x12345678);
 Stream(17, 0x12345678, 0); ExpectStream(a, 7, 0x12345678, 0);
 Stream(17, 0); ExpectStream(a, 7, 0x12345678); // Own context is authoritative, including the low word.
 auto* b = NewRequest(manager, 18, 92);
 Check(Set(b, 0xabcdef01, 0xfedcba98), "second request context accepted");
 Stream(18, 0xfedcba98); ExpectStream(b, 0xabcdef01, 0xfedcba98);
 Check(a->pHeadersReceivedForward->cells.empty(), "second request does not dispatch first request");
 Stream(17, 0x12345678); ExpectStream(a, 7, 0x12345678);
 HTTPRequestCompleted_t completed = {};
 completed.m_hRequest = 17; completed.m_ulContextValue = UINT64_C(0x0000000712345678);
 completed.m_bRequestSuccessful = true; completed.m_eStatusCode = k_EHTTPStatusCode200OK;
 DeliverCall(completed, 91);
 const auto& c = a->pCompletedForward->cells;
 Check(c.size() == 6 && c[4] == 7 && c[5] == 0x12345678 && c[2] == 1 && c[3] == 200, "completion context and status remain provider values");
 auto ha = a->handle, hb = b->handle;
 manager.StopRuntime(); int calls = http.calls;
 Check(!a->SetContextValue(123) && http.calls == calls, "stopped request cannot mutate context or call provider");
 Stream(17, 0); Stream(18, 0);
 Check(steamCallbacks.callbacks.empty(), "shutdown cancels stream dispatch");
 handlesys->Destroy(ha); handlesys->Destroy(hb);
 manager.StartRuntime();
 auto* fresh = NewRequest(manager, 17, 93); // Reused Steam handle, new owner.
 Stream(17, 0); ExpectStream(fresh, 0, 0);
 handlesys->Destroy(fresh->handle);
 Check(steamCallbacks.callbacks.empty() && steamCallbacks.calls.empty() && forwards->owned.empty(), "no callbacks or forwards retained");
 std::printf("PASS: %d x86 streaming context checks (production natives, SDK callback dispatch, independent wire fixture)\n", checks);
}
