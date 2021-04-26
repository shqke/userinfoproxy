#ifndef _INCLUDE_EXTENSION_H_
#define _INCLUDE_EXTENSION_H_

#include "smsdk_ext.h"
#include "wrappers.h"

#include <vector>
#include <iplayerinfo.h>
#include <amtl/am-string.h>

#define USER_INFO_TABLENAME			"userinfo"

#define INVALID_ENT_REFERENCE			( -1 )
#define MAX_PLAYERS						32  // Absolute max players supported

#define USERINFOPROXY_LIB_NAME          "userinfoproxy"

#define USERINFOFLAG_NONE               0x00

#define USERINFOFLAG_SANITIZE_NAME      0x01 // Replace target name with "User #<userid>" (if forward took no action)
#define USERINFOFLAG_HIDE_AVATAR        0x02 // Replace avatar with a blank space
#define USERINFOFLAG_QUERY_FORWARD      0x04 // Query forward UserInfoExt_OnNameQuery for name

class COverride
{
public:
	COverride()
	{
		Reset();
	}

	COverride(IPlugin* pPlugin, int target, int recipient, uint32 uFlags)
	{
		Set(pPlugin, target, recipient, uFlags);
	}

	bool OwnedBy(int client) const
	{
		return m_targetIndex == client || m_recipientIndex == client;
	}

	bool OwnedBy(IPlugin* pPlugin) const
	{
		return m_pPlugin == pPlugin;
	}

	bool Matches(int target, int recipient) const
	{
		return m_targetIndex == target && (IsGlobal() || m_recipientIndex == recipient);
	}

	bool MatchesExact(IPlugin* pPlugin, int target, int recipient) const
	{
		return OwnedBy(pPlugin) && m_targetIndex == target && m_recipientIndex == recipient;
	}

	bool IsFree() const
	{
		return m_uFlags == USERINFOFLAG_NONE;
	}

	bool IsGlobal() const
	{
		return m_recipientIndex == INVALID_ENT_REFERENCE;
	}

	void Reset()
	{
		Set(NULL, INVALID_ENT_REFERENCE, INVALID_ENT_REFERENCE, USERINFOFLAG_NONE);
	}

	void Invalidate()
	{
		OnUpdate();
		Reset();
	}

	void Set(IPlugin* pPlugin, int target, int recipient, uint32 uFlags)
	{
		*this = { pPlugin, target, recipient, uFlags };
		OnUpdate();
	}

	uint32 GetFlags() const
	{
		return m_uFlags;
	}

	void SetFlags(uint32 uFlags)
	{
		if (m_uFlags == uFlags) {
			return;
		}

		m_uFlags = uFlags;
		OnUpdate();
	}

	void OnUpdate()
	{
		if (IsFree()) {
			return;
		}

		extern CNetworkStringTableExt* g_pUserInfoTable;

		if (g_pUserInfoTable != NULL) {
			g_pUserInfoTable->MarkStringAsChanged(m_targetIndex - 1);

			if (!IsGlobal()) {
				g_pUserInfoTable->MarkStringAsChanged(m_recipientIndex - 1);
			}
		}
	}

private:
	IPlugin* m_pPlugin;

	int m_targetIndex : 6;
	int m_recipientIndex : 6;

	// SANITIZE_NAME
	// HIDE_AVATAR
	// QUERY_FORWARD
	uint32 m_uFlags : 3;
};

class CUserInfoProxyExt :
	public SDKExtension,
	public IMetamodListener,
	public IPluginsListener,
	public IClientListener
{
public:
	uint32 GetOverride(int target, int recipient) const;

	uint32 GetOverride(IPlugin* pPlugin, int target, int recipient);
	void SetOverride(IPlugin* pPlugin, int target, int recipient, uint32 uFlags);

	const player_info_t* HandleUserInfo(int target, int recipient, const player_info_t* pPlayerInfo);

protected:
	COverride* FindOverride(IPlugin* pPlugin, int target, int recipient, bool bCreate = false);

	std::vector<COverride> m_Overrides;

protected:
	bool SetupFromGameConfig(IGameConfig* gc, char* error, int maxlength);

public:
	/**
	 * @brief Called when a client is disconnecting (not fully disconnected yet).
	 *
	 * @param client		Index of the client.
	 */
	void OnClientDisconnecting(int client) override;

public:
	// @brief Called when a plugin is about to be unloaded. This is called for
	// any plugin for which OnPluginLoaded was called, and is invoked
	// immediately after OnPluginEnd(). The plugin may be in any state Failed
	// or lower.
	//
	// This function must not cause the plugin to re-enter script code. If
	// you wish to be notified of when a plugin is unloading, and to forbid
	// future calls on that plugin, use OnPluginWillUnload and use a
	// plugin property to block future calls.
	void OnPluginUnloaded(IPlugin* plugin) override;

public:
	/**
	 * @brief Called when the level is loaded (after GameInit, before
	 * ServerActivate).
	 *
	 * To override this, hook IServerGameDLL::LevelInit().
	 *
	 * @param pMapName		Name of the map.
	 * @param pMapEntities	Lump string of the map entities, in KeyValues
	 * 						format.
	 * @param pOldLevel		Unknown.
	 * @param pLandmarkName	Unknown.
	 * @param loadGame		Unknown.
	 * @param background	Unknown.
	 */
	void OnLevelInit(char const* pMapName,
		char const* pMapEntities,
		char const* pOldLevel,
		char const* pLandmarkName,
		bool loadGame,
		bool background) override;

	/**
	 * @brief Called when the level is shut down.  May be called more than
	 * once.
	 */
	void OnLevelShutdown() override;

public:
	/**
	 * @brief This is called after the initial loading sequence has been processed.
	 *
	 * @param error		Error message buffer.
	 * @param maxlength	Size of error message buffer.
	 * @param late		Whether or not the module was loaded after map load.
	 * @return			True to succeed loading, false to fail.
	 */
	bool SDK_OnLoad(char* error, size_t maxlength, bool late) override;

	/**
	 * @brief This is called once the extension unloading process begins.
	 */
	void SDK_OnUnload() override;

public:
	/**
	 * @brief Called when Metamod is attached, before the extension version is called.
	 *
	 * @param error			Error buffer.
	 * @param maxlength		Maximum size of error buffer.
	 * @param late			Whether or not Metamod considers this a late load.
	 * @return				True to succeed, false to fail.
	 */
	bool SDK_OnMetamodLoad(ISmmAPI* ismm, char* error, size_t maxlength, bool late) override;
};

#endif // _INCLUDE_EXTENSION_H_
