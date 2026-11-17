#pragma once
#define SMEXT_CONF_NAME "SteamWorks Capability Fixture (laboratory only)"
#define SMEXT_CONF_DESCRIPTION "Read-only typed interface probes in the isolated laboratory"
#define SMEXT_CONF_VERSION "1.0.0"
#define SMEXT_CONF_AUTHOR "Lechuga"
#define SMEXT_CONF_URL ""
#define SMEXT_CONF_LOGTAG "SWCAPFIXTURE"
#define SMEXT_CONF_LICENSE "GPLv3"
#define SMEXT_CONF_DATESTRING "laboratory"
#define SMEXT_LINK(name) SDKExtension *g_pExtensionIface = name;
#define SMEXT_ENABLE_FORWARDSYS
