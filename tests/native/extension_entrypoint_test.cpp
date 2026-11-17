// Exercise the production SDK entrypoint/wrappers with a minimal SourceMod host.
#define _INCLUDE_SOURCEMOD_EXTENSION_BASESDK_H_
#define SMEXT_ENABLE_HANDLESYS
#define SMEXT_ENABLE_FORWARDSYS
#define SMEXT_ENABLE_PLUGINSYS
#define PLATFORM_EXTERN_C extern "C"
#define SMEXT_CONF_AUTHOR "test"
#define SMEXT_CONF_DATESTRING "test"
#define SMEXT_CONF_DESCRIPTION "test"
#define SMEXT_CONF_VERSION "test"
#define SMEXT_CONF_NAME "test"
#define SMEXT_CONF_LOGTAG "test"
#define SMEXT_CONF_URL "test"
#include <cstdio>
#include <cstdlib>
#include <cstring>

struct IExtension { int identity; };
struct ISourceMod {} sourceMod;
struct IHandleSys {} handleSystem;
struct IForwardManager {} forwardSystem;
namespace SourceMod { struct IPluginManager {} pluginSystem; }
struct IShareSys {
    int acquisitions = 0;
    const char* fail = nullptr;
    IExtension* owners[128] = {};
    template<class T> bool Acquire(const char* name, IExtension* owner, T** result) {
        owners[acquisitions++] = owner;
        if (fail && std::strcmp(name, fail) == 0) return false;
        *result = reinterpret_cast<T*>(&sourceMod);
        return true;
    }
};
#define SM_GET_IFACE(prefix, addr) if (!g_pShareSys->Acquire(#prefix, myself, &addr)) return false
class SDKExtension {
public:
    virtual ~SDKExtension() = default;
    SDKExtension();
    virtual bool SDK_OnLoad(char*, size_t, bool);
    virtual void SDK_OnUnload();
    virtual void SDK_OnAllLoaded();
    virtual void SDK_OnPauseChange(bool);
    virtual void SDK_OnDependenciesDropped();
    bool OnExtensionLoad(IExtension*, IShareSys*, char*, size_t, bool);
    void OnExtensionUnload();
    void OnDependenciesDropped();
    bool IsMetamodExtension();
    void OnExtensionPauseChange(bool);
    void OnExtensionsAllLoaded();
    const char* GetExtensionAuthor();
    const char* GetExtensionDateString();
    const char* GetExtensionDescription();
    const char* GetExtensionVerString();
    const char* GetExtensionName();
    const char* GetExtensionTag();
    const char* GetExtensionURL();
};
using IExtensionInterface = SDKExtension;
extern SDKExtension* g_pExtensionIface;
#ifndef SDK_ENTRY_SOURCE
#define SDK_ENTRY_SOURCE "../../extension/sdk/smsdk_ext.cpp"
#endif
#include SDK_ENTRY_SOURCE

int checks = 0;
void Check(bool ok, const char* why) {
    ++checks;
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", why); std::exit(1); }
}
struct Extension final : SDKExtension {
    int loads = 0, unloads = 0, listeners = 0;
    IExtension* resourceOwner = nullptr;
    bool fail = false;
    void (*duringLoad)() = nullptr;
    void (*duringUnload)() = nullptr;
    void (*duringDrop)() = nullptr;
    bool SDK_OnLoad(char*, size_t, bool) override {
        ++loads;
        if (duringLoad) duringLoad();
        if (fail) return false;
        ++listeners; resourceOwner = myself;
        return true;
    }
    void SDK_OnUnload() override {
        ++unloads;
        Check(myself == resourceOwner, "teardown keeps original identity");
        if (duringUnload) duringUnload();
        --listeners; resourceOwner = nullptr;
    }
    void SDK_OnDependenciesDropped() override { if (duringDrop) duringDrop(); }
} extension;
SDKExtension* g_pExtensionIface = &extension;

struct Loader {
    int metamodAttaches = 0, metamodDetaches = 0;
    bool Load(IExtension& owner, IShareSys& host) {
        // SourceMod validates GetSMExtAPI before touching Metamod. A rejected
        // factory must not acquire/detach an already attached Metamod ID.
        auto* api = GetSMExtAPI();
        if (!api) return false;
        ++metamodAttaches;
        char error[128] = {};
        if (!api->OnExtensionLoad(&owner, &host, error, sizeof(error), true)) {
            ++metamodDetaches;
            return false;
        }
        return true;
    }
};
IExtension *firstContext, *secondContext;
IShareSys *hostContext, *otherContext;
int main() {
    IExtension physical{11}, logical{22}; IShareSys host, other;
    // Same DSO, both ordering directions, plus unload/reload without unmapping.
    for (bool physicalFirst : {true, false}) {
        IExtension& first = physicalFirst ? physical : logical;
        IExtension& second = physicalFirst ? logical : physical;
        Loader loader;
        Check(GetSMExtAPI() == &extension, "fresh factory returns original ABI instance");
        firstContext = &first; secondContext = &second; hostContext = &host; otherContext = &other;
        extension.duringLoad = [] {
            Check(GetSMExtAPI() == nullptr, "factory blocks reentry during initialization");
            char error[64] = {};
            Check(!extension.OnExtensionLoad(secondContext, otherContext, error, sizeof(error), false), "wrapper rejects direct nested load");
            Check(std::strstr(error, "load owner") && myself == firstContext && sharesys == hostContext, "nested rejection preserves SDK globals");
        };
        Check(loader.Load(first, host), "first load succeeds");
        const auto acquisitions = host.acquisitions; const auto loads = extension.loads;
        auto* originalForwards = forwards; auto* originalHandles = handlesys; auto* originalPlugins = plsys;
        for (int attempt = 0; attempt != 3; ++attempt) {
            Check(!loader.Load(second, other), "duplicate factory rejected before Metamod");
            char tiny[1] = {'x'};
            Check(!extension.OnExtensionLoad(&second, &other, tiny, sizeof(tiny), true) && tiny[0] == 0, "direct rejection respects tiny error buffer");
            Check(!extension.OnExtensionLoad(&second, &other, nullptr, 0, true), "direct rejection permits no error buffer");
            Check(myself == &first && sharesys == &host && g_pShareSys == &host, "owner and host cannot be overwritten");
            Check(forwards == originalForwards && handlesys == originalHandles && plsys == originalPlugins, "service interfaces preserved");
            Check(extension.loads == loads && extension.listeners == 1 && extension.resourceOwner == &first, "no second resources or orphaned listener");
            Check(host.acquisitions == acquisitions && other.acquisitions == 0, "duplicate rejection precedes interface acquisition");
        }
        char untouched = 'x';
        Check(!extension.OnExtensionLoad(&first, &host, &untouched, 0, true) && untouched == 'x', "same-owner duplicate and zero capacity cannot mutate state or output");
        Check(loader.metamodAttaches == 1 && loader.metamodDetaches == 0, "duplicate cannot detach original Metamod attachment");
        extension.duringUnload = [] { Check(GetSMExtAPI() == nullptr, "factory stays closed during resource teardown"); };
        extension.duringDrop = [] { Check(GetSMExtAPI() == nullptr, "factory stays closed while bindings are dropped"); };
        extension.OnExtensionUnload();
        Check(extension.listeners == 0 && GetSMExtAPI() == nullptr, "no listener survives unload; dependency phase still closed");
        Check(!extension.OnExtensionLoad(&second, &other, nullptr, 0, true) && myself == &first, "direct load stays blocked until bindings are dropped");
        const auto unloads = extension.unloads;
        extension.OnExtensionUnload(); Check(extension.unloads == unloads, "duplicate unload is inert");
        extension.OnDependenciesDropped();
        Check(myself == nullptr && GetSMExtAPI() == &extension, "completed teardown permits a new load");
    }
    extension.duringLoad = {};
    for (const char* name : {"SOURCEMOD", "HANDLESYSTEM", "FORWARDMANAGER", "PLUGINSYSTEM"}) {
        host.fail = name;
        Check(!extension.OnExtensionLoad(&logical, &host, nullptr, 0, false), "missing required interface fails load");
        Check(myself == nullptr && GetSMExtAPI() == &extension && extension.listeners == 0, "early failure releases claim without orphaned ownership");
    }
    host.fail = nullptr; extension.fail = true;
    Check(!extension.OnExtensionLoad(&logical, &host, nullptr, 0, false), "SDK load failure propagates");
    Check(myself == nullptr && GetSMExtAPI() == &extension, "SDK failure releases claim for retry");
    extension.fail = false;
    Check(extension.OnExtensionLoad(&logical, &host, nullptr, 0, true), "retry after failed initialization succeeds");
    extension.OnExtensionUnload(); extension.OnDependenciesDropped();
    Check(extension.listeners == 0, "last owner removes listener exactly once");
    std::printf("PASS: %d extension entrypoint checks (production SDK wrapper, simulated loader/host)\n", checks);
}
