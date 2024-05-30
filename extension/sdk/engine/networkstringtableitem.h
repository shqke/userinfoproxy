//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef NETWORKSTRINGTABLEITEM_H
#define NETWORKSTRINGTABLEITEM_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

class CNetworkStringTableItem
{
public:
	enum
	{
		MAX_USERDATA_BITS = 14,
		MAX_USERDATA_SIZE = (1 << MAX_USERDATA_BITS)
	};

	inline int		GetTickCreated(void) const { return m_nTickCreated; }
	inline int		GetTickChanged(void) const { return m_nTickChanged; }

	struct itemchange_s {
		int				tick;
		int				length;
		unsigned char	*data;
	};

public:
	unsigned char	*m_pUserData;
	int				m_nUserDataLength;
	int				m_nTickChanged;

#ifndef SHARED_NET_STRING_TABLES
	int				m_nTickCreated;
	CUtlVector< itemchange_s > *m_pChangeList;	
#endif
};

#endif // NETWORKSTRINGTABLEITEM_H
