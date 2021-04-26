#include "extension.h"
#include "natives.h"

extern CUserInfoProxyExt g_UserInfoProxyExt;

static bool ValidatePlayer(int client, IPluginContext* pContext)
{
	IGamePlayer* pGamePlayer = playerhelpers->GetGamePlayer(client);
	if (pGamePlayer == NULL) {
		pContext->ThrowNativeError("Client index %d is invalid", client);
		return false;
	}

	if (!pGamePlayer->IsConnected()) {
		pContext->ThrowNativeError("Client %d is not connected", client);
		return false;
	}

	return true;
}

// native int GetOverride(int target, int recipient = INVALID_ENT_REFERENCE, bool sum = false);
cell_t Handler_GetOverride(IPluginContext* pContext, const cell_t* params)
{
	if (!ValidatePlayer(params[1], pContext)) {
		return 0;
	}

	if (params[2] != INVALID_ENT_REFERENCE && !ValidatePlayer(params[2], pContext)) {
		return 0;
	}

	bool bSumOverride = params[3] != 0;
	if (bSumOverride) {
		return g_UserInfoProxyExt.GetOverride(params[1], params[2]);
	}

	IPlugin* pPlugin = plsys->FindPluginByContext(pContext->GetContext());
	return g_UserInfoProxyExt.GetOverride(pPlugin, params[1], params[2]);
}

// native void SetOverride(int target, int flags, int recipient = INVALID_ENT_REFERENCE);
cell_t Handler_SetOverride(IPluginContext* pContext, const cell_t* params)
{
	if (!ValidatePlayer(params[1], pContext)) {
		return 0;
	}

	if (params[3] != INVALID_ENT_REFERENCE && !ValidatePlayer(params[3], pContext)) {
		return 0;
	}

	IPlugin* pPlugin = plsys->FindPluginByContext(pContext->GetContext());
	g_UserInfoProxyExt.SetOverride(pPlugin, params[1], params[3], params[2]);

	return 0;
}

const sp_nativeinfo_t g_Natives[] =
{
	{ "UserInfoExt.GetOverride", Handler_GetOverride },
	{ "UserInfoExt.SetOverride", Handler_SetOverride },

	{ NULL, NULL },
};
