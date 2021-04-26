#ifndef _INCLUDE_WRAPPERS_H_
#define _INCLUDE_WRAPPERS_H_

#include "smsdk_ext.h"

#include <iclient.h>
#include <igameevents.h>
#include <cdll_int.h>

#include "sdk/engine/networkstringtable.h"
#include "sdk/engine/networkstringtableitem.h"

extern IServerGameEnts* gameents;

class CBaseEntity :
	public IServerEntity
{
public:
	edict_t* edict()
	{
		return gameents->BaseEntityToEdict(this);
	}

	int entindex()
	{
		return gamehelpers->EntityToBCompatRef(this);
	}

	const char* GetClassName()
	{
		return edict()->GetClassName();
	}
};

inline CBaseEntity* GetContainingEntity(edict_t* pent)
{
	return (CBaseEntity*)gameents->EdictToBaseEntity(pent);
}

class CBaseClient :
	public IGameEventListener2,
	public IClient,
	public IClientMessageHandler
{
public:
};

class CNetworkStringTableExt :
	public CNetworkStringTable
{
public:
	void MarkStringAsChanged(int stringNumber = -1)
	{
		extern CGlobalVars* gpGlobals;
		m_nLastChangedTick = gpGlobals->tickcount;

		if (stringNumber == -1) {
			int count = m_pItems->Count();
			for (int i = 0; i < count; i++) {
				CNetworkStringTableItem* pItem = &m_pItems->Element(i);
				pItem->m_nTickChanged = gpGlobals->tickcount;
			}

			return;
		}

		CNetworkStringTableItem* pItem = &m_pItems->Element(stringNumber);
		pItem->m_nTickChanged = gpGlobals->tickcount;
	}
};

#endif // _INCLUDE_WRAPPERS_H_
