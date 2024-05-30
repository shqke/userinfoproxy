#include "extension.h"
#include "wrappers.h"
#include "natives.h"

#include <CDetour/detours.h>

CUserInfoProxyExt g_UserInfoProxyExt;
SMEXT_LINK(&g_UserInfoProxyExt);

// Interfaces
INetworkStringTableContainer* networkStringTableContainerServer = NULL;
IServerGameEnts* gameents = NULL;

CGlobalVars* gpGlobals = NULL;
CNetworkStringTableExt* g_pUserInfoTable = NULL;

void* pfn_CNetworkStringTable_WriteUpdate = NULL;
CDetour* detour_CNetworkStringTable_WriteUpdate = NULL;

IForward* g_pQueryForward = NULL;

DETOUR_DECL_MEMBER3(Handler_CNetworkStringTable_WriteUpdate, int, CBaseClient*, pClient, bf_write&, buf, int, tick_ack)
{
	CNetworkStringTableExt* _this = reinterpret_cast<CNetworkStringTableExt*>(this);
	if (_this != g_pUserInfoTable || pClient == NULL) {
		return DETOUR_MEMBER_CALL(Handler_CNetworkStringTable_WriteUpdate)(pClient, buf, tick_ack);
	}

	int iRecipient = pClient->GetPlayerSlot() + 1;

	int entriesUpdated = 0;
	int lastEntry = -1;

	// shqke: not using dictionaries
	buf.WriteOneBit(0);

	int maxCount = MIN(_this->m_pItems->Count(), MAX_PLAYERS);
	for (int i = 0; i < maxCount; i++) {
		// rww: dont need to send an update if overriden and haven't changed
		CNetworkStringTableItem* pItem = &_this->m_pItems->Element(i);
		if (pItem->GetTickChanged() <= tick_ack) {
			// Client is up to date
			continue;
		}

		// Write Entry index
		if ((lastEntry + 1) == i) {
			buf.WriteOneBit(1);
		}
		else {
			buf.WriteOneBit(0);
			buf.WriteUBitLong(i, _this->GetEntryBits());
		}

		if (pItem->GetTickCreated() >= tick_ack) {
			// this item has just been created, send string itself
			buf.WriteOneBit(1);

			// shqke: not using string history for indices
			buf.WriteOneBit(0);

			char const* pEntry = _this->m_pItems->String(i);
			buf.WriteString(pEntry);
		}
		else {
			buf.WriteOneBit(0);
		}

		// Write the item's user data.
		int length = 0;
		const player_info_t* pPlayerInfo = static_cast<const player_info_t*>(_this->GetStringUserData(i, &length));
		if (pPlayerInfo != NULL && length > 0) {
			pPlayerInfo = g_UserInfoProxyExt.HandleUserInfo(i + 1, iRecipient, pPlayerInfo);

			buf.WriteOneBit(1);

			// shqke: "userinfo" items are not fixed in size
			buf.WriteUBitLong(length, CNetworkStringTableItem::MAX_USERDATA_BITS);
			buf.WriteBits(pPlayerInfo, length * 8);
		}
		else {
			buf.WriteOneBit(0);
		}

		entriesUpdated++;
		lastEntry = i;
	}

	return entriesUpdated;
}

uint32 CUserInfoProxyExt::GetOverride(int target, int recipient) const
{
	uint32 uFlags = USERINFOFLAG_NONE;

	for (auto& el : m_Overrides) {
		if (!el.Matches(target, recipient)) {
			continue;
		}

		uFlags |= el.GetFlags();
	}

	return uFlags;
}

uint32 CUserInfoProxyExt::GetOverride(IPlugin* pPlugin, int target, int recipient)
{
	COverride* pOverride = FindOverride(pPlugin, target, recipient);
	if (pOverride != NULL) {
		return pOverride->GetFlags();
	}

	return USERINFOFLAG_NONE;
}

void CUserInfoProxyExt::SetOverride(IPlugin* pPlugin, int target, int recipient, uint32 uFlags)
{
	COverride* pOverride = FindOverride(pPlugin, target, recipient, true);
	if (pOverride != NULL) {
		if (uFlags == USERINFOFLAG_NONE) {
			pOverride->Invalidate();
			return;
		}

		pOverride->SetFlags(uFlags);
	}
}

const player_info_t* CUserInfoProxyExt::HandleUserInfo(int target, int recipient, const player_info_t* pPlayerInfo)
{
	uint32 uFlags = GetOverride(target, recipient);
	if (uFlags == USERINFOFLAG_NONE) {
		return pPlayerInfo;
	}

	static player_info_t s_info;
	s_info = *pPlayerInfo;

	do {
		if ((uFlags & USERINFOFLAG_QUERY_FORWARD) != 0) {
			char szName[MAX_PLAYER_NAME_LENGTH];
			ke::SafeStrcpy(szName, sizeof(szName), s_info.name);

			extern IForward* g_pQueryForward;

			ResultType result = Pl_Continue;

			g_pQueryForward->PushCell(target);
			g_pQueryForward->PushCell(recipient);
			g_pQueryForward->PushStringEx(szName, sizeof(szName), SM_PARAM_STRING_UTF8, SM_PARAM_COPYBACK);
			g_pQueryForward->PushCell(sizeof(szName));
			if (g_pQueryForward->Execute((cell_t*)&result) == SP_ERROR_NONE && result >= Pl_Changed) {
				ke::SafeStrcpy(s_info.name, sizeof(s_info.name), szName);
				break;
			}
		}

		if ((uFlags & USERINFOFLAG_SANITIZE_NAME) != 0) {
			ke::SafeSprintf(s_info.name, sizeof(s_info.name), "User #%u", BigLong(pPlayerInfo->userID));
		}
	} while (false);

	if ((uFlags & USERINFOFLAG_HIDE_AVATAR) != 0) {
		// Hides avatar
		s_info.friendsID = 1ull;
	}

	return &s_info;
}

COverride* CUserInfoProxyExt::FindOverride(IPlugin* pPlugin, int target, int recipient, bool bCreate)
{
	COverride* pFirstFree = NULL;

	for (auto& el : m_Overrides) {
		if (el.MatchesExact(pPlugin, target, recipient)) {
			return &el;
		}

		if (bCreate && pFirstFree == NULL && el.IsFree()) {
			pFirstFree = &el;
		}
	}

	if (bCreate) {
		if (pFirstFree == NULL) {
			m_Overrides.emplace_back(pPlugin, target, recipient, USERINFOFLAG_NONE);
			return &m_Overrides.back();
		}

		pFirstFree->Set(pPlugin, target, recipient, USERINFOFLAG_NONE);
	}

	return pFirstFree;
}

bool CUserInfoProxyExt::SetupFromGameConfig(IGameConfig* gc, char* error, int maxlength)
{
	static const struct {
		const char* key;
		void*& address;
	} s_sigs[] = {
		{ "CNetworkStringTable::WriteUpdate", pfn_CNetworkStringTable_WriteUpdate },
	};

	for (auto&& el : s_sigs) {
		if (!gc->GetMemSig(el.key, &el.address)) {
			ke::SafeSprintf(error, maxlength, "Unable to find signature for \"%s\" from game config (file: \"" GAMEDATA_FILE ".txt\")", el.key);

			return false;
		}

		if (el.address == NULL) {
			ke::SafeSprintf(error, maxlength, "Sigscan for \"%s\" failed (game config file: \"" GAMEDATA_FILE ".txt\")", el.key);

			return false;
		}
	}

	return true;
}

void CUserInfoProxyExt::OnClientDisconnecting(int client)
{
	for (auto& el : m_Overrides) {
		if (el.OwnedBy(client)) {
			el.Invalidate();
		}
	}
}

void CUserInfoProxyExt::OnPluginUnloaded(IPlugin* plugin)
{
	for (auto& el : m_Overrides) {
		if (el.OwnedBy(plugin)) {
			el.Invalidate();
		}
	}
}

void CUserInfoProxyExt::OnLevelInit(char const* pMapName, char const* pMapEntities, char const* pOldLevel, char const* pLandmarkName, bool loadGame, bool background)
{
	g_pUserInfoTable = static_cast<CNetworkStringTableExt*>(networkStringTableContainerServer->FindTable(USER_INFO_TABLENAME));
}

void CUserInfoProxyExt::OnLevelShutdown()
{
	g_pUserInfoTable = NULL;
}

bool CUserInfoProxyExt::SDK_OnLoad(char* error, size_t maxlength, bool late)
{
	g_pUserInfoTable = static_cast<CNetworkStringTableExt*>(networkStringTableContainerServer->FindTable(USER_INFO_TABLENAME));

	IGameConfig* gc = NULL;
	if (!gameconfs->LoadGameConfigFile(GAMEDATA_FILE, &gc, error, maxlength)) {
		ke::SafeStrcpy(error, maxlength, "Unable to load a gamedata file \"" GAMEDATA_FILE ".txt\"");

		return false;
	}

	if (!SetupFromGameConfig(gc, error, maxlength)) {
		gameconfs->CloseGameConfigFile(gc);

		return false;
	}

	gameconfs->CloseGameConfigFile(gc);

	// Game config is never used by detour class to handle errors ourselves
	CDetourManager::Init(smutils->GetScriptingEngine(), NULL);

	detour_CNetworkStringTable_WriteUpdate = DETOUR_CREATE_MEMBER(Handler_CNetworkStringTable_WriteUpdate, pfn_CNetworkStringTable_WriteUpdate);
	if (detour_CNetworkStringTable_WriteUpdate == NULL) {
		ke::SafeStrcpy(error, maxlength, "Unable to create a detour for \"CNetworkStringTable::WriteUpdate\"");

		return false;
	}

	detour_CNetworkStringTable_WriteUpdate->EnableDetour();

	ParamType params[]{ Param_Cell, Param_Cell, Param_String, Param_Cell };
	g_pQueryForward = forwards->CreateForward("UserInfoExt_OnNameQuery", ExecType::ET_Hook, ARRAYSIZE(params), params);

	playerhelpers->AddClientListener(this);
	plsys->AddPluginsListener(this);

	sharesys->AddNatives(myself, g_Natives);

	sharesys->RegisterLibrary(myself, USERINFOPROXY_LIB_NAME);

	return true;
}

void CUserInfoProxyExt::SDK_OnUnload()
{
	if (detour_CNetworkStringTable_WriteUpdate != NULL) {
		detour_CNetworkStringTable_WriteUpdate->Destroy();
		detour_CNetworkStringTable_WriteUpdate = NULL;
	}

	if (g_pUserInfoTable != NULL) {
		g_pUserInfoTable->MarkStringAsChanged(-1);
	}

	forwards->ReleaseForward(g_pQueryForward);
	plsys->RemovePluginsListener(this);
	playerhelpers->RemoveClientListener(this);
}

bool CUserInfoProxyExt::SDK_OnMetamodLoad(ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
	GET_V_IFACE_CURRENT(GetServerFactory, gameents, IServerGameEnts, INTERFACEVERSION_SERVERGAMEENTS);
	GET_V_IFACE_CURRENT(GetEngineFactory, networkStringTableContainerServer, INetworkStringTableContainer, INTERFACENAME_NETWORKSTRINGTABLESERVER);

	gpGlobals = ismm->GetCGlobals();

	ismm->AddListener(g_PLAPI, this);

	return true;
}
