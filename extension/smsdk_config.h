#ifndef _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_

#define SMEXT_CONF_NAME			"userinfo proxy"
#define SMEXT_CONF_DESCRIPTION	"Allows to alter userinfo data to individual players"
#define SMEXT_CONF_VERSION		"1.0"
#define SMEXT_CONF_AUTHOR		"Evgeniy \"shqke\" Kazakov"
#define SMEXT_CONF_URL			"https://github.com/shqke/userinfoproxy"
#define SMEXT_CONF_LOGTAG		"userinfoproxy"
#define SMEXT_CONF_LICENSE		"GPL"
#define SMEXT_CONF_DATESTRING	__DATE__

#define GAMEDATA_FILE			"userinfoproxy"

#define SMEXT_LINK(name) SDKExtension *g_pExtensionIface = name;

#define SMEXT_CONF_METAMOD
#define SMEXT_ENABLE_GAMEHELPERS
#define SMEXT_ENABLE_GAMECONF
#define SMEXT_ENABLE_PLAYERHELPERS
#define SMEXT_ENABLE_PLUGINSYS
#define SMEXT_ENABLE_FORWARDSYS

#endif // _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_
