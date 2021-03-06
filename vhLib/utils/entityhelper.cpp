
#include "vh.h"

#include "bytescanner.h"
#include "entityhelper.h"

#include "cbase.h"



CEntityHelper &EntityHelper()
{
	static CEntityHelper entityHelper;
	return entityHelper;
}

void CEntityHelper::Init()
{ 
	CByteScanner byteScan( "client" );

	// we're sigscanning the cl_dota_showents concommand handler
	// which references CBaseEntityList's m_EntInfoArray

	void *pFunc = NULL;
	if ( byteScan.FindCodePattern( "\x55\x8B\xEC\x83\xEC\x10\xA1\x00\x00\x00\x00\x8B\x50\x18", "xxxxxxx????xxx", &pFunc ) )
	{
		// m_EntInfoArray's access is inlined 0x67 bytes into the func
		m_pEntInfo = *(CEntInfo **)( (uint8 *)pFunc + 0x67 );
	}
	else
	{
		Warning( "[EntityHelper] Unable to find cl_dota_showents!\n" );
	}


	// we find g_pGameRules relative to CDOTA_Modifier_Roshan_CandyBuff::GetActivityTranslationModifiers
	// cause why not
	
	if ( byteScan.FindCodePattern( "\x55\x8B\xEC\x8B\x49\x78\x83\xF9\xFF\x74\x51", "xxxxxxxxxxx", &pFunc ) )
	{
		m_pGameRules = *(C_GameRules ***)( (uint8 *)pFunc + 0x2A );
	}
	else
	{
		Warning( "[EntityHelper] Unable to find CDOTA_Modifier_Roshan_CandyBuff::GetActivityTranslationModifiers!\n" );
	}
}

void CEntityHelper::Shutdown()
{
	m_pEntInfo = NULL;
	m_pGameRules = NULL;

	FOR_EACH_MAP_FAST( m_RecvPropCache, i )
	{
		delete m_RecvPropCache.Element( i );
	}

	FOR_EACH_MAP_FAST( m_DataMapCache, i )
	{
		delete m_DataMapCache.Element( i );
	}
}

C_BasePlayer *CEntityHelper::GetLocalPlayer()
{
	// IClientTools::GetLocalPlayer returns the C_BasePlayer casted to a EntitySearchResult
	return reinterpret_cast<C_BasePlayer *>( VH().ClientTools()->GetLocalPlayer() );
}

C_BaseEntity *CEntityHelper::GetEntityFromIndex( int entIndex )
{
	if ( m_pEntInfo == NULL )
		return NULL; // can't get an entity if we don't have the entlist

	if ( entIndex < 0 || entIndex >= NUM_ENT_ENTRIES )
		return NULL;

	IClientUnknown *pEnt = reinterpret_cast<IClientUnknown *>( m_pEntInfo[ entIndex ].m_pEntity );

	if ( !pEnt )
		return NULL; // no ent in this index

	return reinterpret_cast<C_BaseEntity *>( pEnt->GetBaseEntity() );
}
IHandleEntity *CEntityHelper::LookupEntity( const CBaseHandle &handle )
{
	if ( m_pEntInfo == NULL )
		return NULL;

	if ( !handle.IsValid() )
		return NULL;

	CEntInfo *pInfo = &m_pEntInfo[ handle.GetEntryIndex() ];

	if ( pInfo->m_SerialNumber == handle.GetSerialNumber() )
		return pInfo->m_pEntity;

	return NULL;
}

C_BaseEntity *CEntityHelper::GetResourceEntity()
{
	if ( m_ResourceEntity.Get() == NULL )
	{
		m_ResourceEntity.Set( FindEntityByNetClass( "CDOTA_PlayerResource" ) );
	}

	IClientUnknown *pUnk = reinterpret_cast<IClientUnknown *>( m_ResourceEntity.Get() );

	if ( !pUnk )
		return NULL;

	return pUnk->GetBaseEntity();
}

C_BaseEntity *CEntityHelper::GetGameRulesProxyEntity()
{
	if ( m_GameRulesProxyEntity.Get() == NULL )
	{
		m_GameRulesProxyEntity.Set( FindEntityByNetClass( "CDOTAGamerulesProxy" ) );
	}

	IClientUnknown *pUnk = reinterpret_cast<IClientUnknown *>( m_GameRulesProxyEntity.Get() );
	
	if ( !pUnk )
		return NULL;
	
	return pUnk->GetBaseEntity();
}

C_GameRules *CEntityHelper::GetGameRules()
{
	if ( m_pGameRules == NULL )
		return NULL;

	return *m_pGameRules;
}


bool CEntityHelper::GetEntPropArraySize( C_BaseEntity *pEnt, EntPropType propType, const char *propName, int *outSize )
{
	switch ( propType )
	{
		case EntProp_RecvProp:
		{
			RecvPropInfo_t propInfo;

			if ( !GetRecvPropInfo( pEnt, propName, &propInfo ) )
				return false;

			if ( propInfo.prop->GetType() != DPT_DataTable )
				return false; // array props must be datatables

			RecvTable *pTable = propInfo.prop->GetDataTable();
			Assert( pTable );

			*outSize = pTable->GetNumProps();
			return true;
		}
		
		case EntProp_DataMap:
		{
			DataMapInfo_t mapInfo;

			if ( !GetDataMapInfo( pEnt, propName, &mapInfo ) )
			{
				// if we can't find the datamap on the entity we're checking, check against worldspawn
				if ( !GetDataMapInfo( GetEntityFromIndex( 0 ), propName, &mapInfo ) )
					return false;
			}

			*outSize = mapInfo.prop->fieldSize;
			return true;
		}

		default:
			Assert( !"Unknown ent prop type in GetEntPropArraySize!" );
	}

	return false;
}

bool FindInRecvTable( RecvTable *pTable, const char *propName, RecvPropInfo_t *pInfo, int startOffset )
{
	Assert( pTable );

	int numProps = pTable->GetNumProps();

	for ( int x = 0 ; x < numProps ; ++x )
	{
		RecvProp *pProp = pTable->GetProp( x );
		const char *name = pProp->GetName();

		if ( name && V_strcmp( name, propName ) == 0 )
		{
			pInfo->actualOffset = startOffset + pProp->GetOffset();
			pInfo->prop = pProp;
			return true;
		}

		if ( pProp->GetDataTable() )
		{
			if ( FindInRecvTable( pProp->GetDataTable(), propName, pInfo, startOffset + pProp->GetOffset() ) )
				return true;
		}
	}

	return false;
}

bool FindInDataMap( datamap_t *pDataMap, const char *dataName, DataMapInfo_t *pInfo )
{
	Assert( pDataMap );

	while ( pDataMap )
	{
		for ( int i = 0 ; i < pDataMap->dataNumFields ; ++i )
		{
			typedescription_t *pDesc = &( pDataMap->dataDesc[ i ] );

			if ( pDesc->fieldName == NULL )
				continue;

			if ( V_strcmp( dataName, pDesc->fieldName ) == 0 )
			{
				pInfo->prop = pDesc;
				pInfo->actualOffset = pDesc->fieldOffset;
				return true;
			}

			if ( pDesc->td == NULL )
				continue;

			if ( !FindInDataMap( pDesc->td, dataName, pInfo ) )
				continue;

			pInfo->actualOffset += pDesc->fieldOffset;
			return true;
		}

		pDataMap = pDataMap->baseMap;
	}

	return false;
}

bool CEntityHelper::GetRecvPropInfo( IClientNetworkable *pNetworkable, const char *propName, RecvPropInfo_t *pInfo )
{
	if ( pNetworkable == NULL )
		return false;

	ClientClass *pClass = pNetworkable->GetClientClass();
	Assert( pClass );

	if ( pClass == NULL )
		return false;

	auto classIndex = m_RecvPropCache.Find( pClass );

	if ( !m_RecvPropCache.IsValidIndex( classIndex ) )
		classIndex = m_RecvPropCache.Insert( pClass, new Cache_t<RecvPropInfo_t>() );

	Cache_t<RecvPropInfo_t> &cache = *m_RecvPropCache[ classIndex ];

	auto cacheIndex = cache.Find( propName );

	if ( cache.IsValidIndex( cacheIndex ) )
	{
		*pInfo = cache[ cacheIndex ];
		return true;
	}

	if ( !FindInRecvTable( pClass->m_pRecvTable, propName, pInfo, 0 ) )
		return false; // couldn't find it at all

	cache.Insert( propName, *pInfo );
	return true;
}

bool CEntityHelper::GetDataMapInfo( C_BaseEntity *pEntity, const char *dataName, DataMapInfo_t *pInfo )
{
	if ( pEntity == NULL )
		return false;

	datamap_t *pDataMap = pEntity->GetDataDescMap();

	if ( pDataMap == NULL )
		return false;

	auto mapIndex = m_DataMapCache.Find( pDataMap );

	if ( !m_DataMapCache.IsValidIndex( mapIndex ) )
		mapIndex = m_DataMapCache.Insert( pDataMap, new Cache_t<DataMapInfo_t>() );

	Cache_t<DataMapInfo_t> &cache = *m_DataMapCache[ mapIndex ];

	auto cacheIndex = cache.Find( dataName );

	if ( cache.IsValidIndex( cacheIndex ) )
	{
		*pInfo = cache[ cacheIndex ];
		return true;
	}

	if ( !FindInDataMap( pDataMap, dataName, pInfo ) )
		return false;

	cache.Insert( dataName, *pInfo );
	return true;
}

C_BaseEntity *CEntityHelper::FindEntityByNetClass( const char *netClass )
{
	for ( int i = 0 ; i < MAX_EDICTS ; ++i )
	{
		C_BaseEntity *pEnt = GetEntityFromIndex( i );

		if ( !pEnt )
			continue;

		ClientClass *pClass = pEnt->GetClientClass();

		if ( !pClass || !pClass->m_pNetworkName )
			continue;

		if ( V_strcasecmp( pClass->m_pNetworkName, netClass ) == 0 )
		{
			return pEnt;
		}
	}

	return NULL;
}
