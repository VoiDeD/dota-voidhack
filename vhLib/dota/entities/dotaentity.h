

#pragma once


#include "entprop.h"
#include "entfuncs.h"

#include "dota_consts.h"

#include "networkvar.h"


class C_BaseEntity;


// base wrapper for most dota related entities
class C_DOTABaseEntity
{
	DECLARE_CLASS_NOBASE( C_DOTABaseEntity );

public:
	C_DOTABaseEntity( C_BaseEntity *pEnt );

	operator C_BaseEntity *() { return GetEntity(); }


	// classname
	const char *GetEntityName() { return m_iName; }


	// return the underlying entity this class wraps
	// this is required to support entprop lookup!
	C_BaseEntity *GetEntity() const
	{
		IClientUnknown *pUnk = reinterpret_cast<IClientUnknown *>( m_Entity.Get() );

		if ( !pUnk )
			return NULL;

		return pUnk->GetBaseEntity();
	}

	// is the wrapped entity valid?
	bool IsValid()
	{
		C_BaseEntity *pEnt = GetEntity();

		if ( !pEnt )
			return false;

		// todo: re-enable when we can verify we're calling the right function
		/*
		if ( pEnt->IsDormant() )
			return false; // outside of PVS
		*/

		return true;
	}


	CEntProp( char *, m_iName );


private:
	CBaseHandle m_Entity;

};
