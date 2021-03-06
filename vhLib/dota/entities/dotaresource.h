
#pragma once


#include "dotaentity.h"


// wrapper over the global player resource entity
class C_DOTAResource : public C_DOTABaseEntity
{
	DECLARE_CLASS( C_DOTAResource, C_DOTABaseEntity );

public:
	C_DOTAResource( C_BaseEntity *pEnt );


	CEntPropArray( int, m_nSelectedHeroID );

	CEntPropArray( int, m_iKills );
	CEntPropArray( int, m_iAssists );
	CEntPropArray( int, m_iDeaths );

	CEntPropArray( int, m_iLevel );


	static C_DOTAResource GetResourceEntity();

};
