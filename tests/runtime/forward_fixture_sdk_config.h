#pragma once
#define SMEXT_CONF_NAME "SteamWorks Forward Fixture (laboratory only)"
#define SMEXT_CONF_DESCRIPTION "Synthetic dispatch through existing managed forwards; no Steam calls"
#define SMEXT_CONF_VERSION "1.0.0"
#define SMEXT_CONF_AUTHOR "Lechuga"
#define SMEXT_CONF_URL ""
#define SMEXT_CONF_LOGTAG "SWFORWARDFIXTURE"
#define SMEXT_CONF_LICENSE "GPLv3"
#define SMEXT_CONF_DATESTRING "laboratory"
#define SMEXT_LINK(name) SDKExtension *g_pExtensionIface = name;
#define SMEXT_ENABLE_FORWARDSYS
