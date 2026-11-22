#ifndef IKTBMATTRIBUTES_H
#define IKTBMATTRIBUTES_H
#ifdef _WIN32
#pragma once
#endif

class CBaseEntity;

#define KTBM_ATTRIBUTES_INTERFACE_VERSION "KTBMAttributes001"

class IKTBMAttributes
{
public:

	virtual int GetAttributeDefIndex( const char *pszName ) = 0;
	virtual bool GetAttributeName( int nDefIndex, char *pszName, int cchName ) = 0;
	virtual bool GetAttributeClass( int nDefIndex, char *pszClass, int cchClass ) = 0;
	virtual bool IsAttributeNumber( int nDefIndex ) = 0;
	virtual bool IsAttributeStoredAsInteger( int nDefIndex ) = 0;
	virtual float HookValueFloat( CBaseEntity *pEntity, const char *pszAttribClass, float flBase ) = 0;
	virtual int HookValueInt( CBaseEntity *pEntity, const char *pszAttribClass, int nBase ) = 0;
	virtual bool HookValueString( CBaseEntity *pEntity, const char *pszAttribClass, const char *pszBase, char *pszValue, int cchValue ) = 0;
	virtual bool HasAttributes( CBaseEntity *pEntity ) = 0;
	virtual bool SetRuntimeAttribute( CBaseEntity *pEntity, int nDefIndex, float flValue ) = 0;
	virtual bool RemoveRuntimeAttribute( CBaseEntity *pEntity, int nDefIndex ) = 0;
	virtual bool GetRuntimeAttribute( CBaseEntity *pEntity, int nDefIndex, float *pflValue ) = 0;
	virtual int GetRuntimeAttributes( CBaseEntity *pEntity, int *pnDefIndexes, float *pflValues, int nMax ) = 0;
	virtual void RemoveAllRuntimeAttributes( CBaseEntity *pEntity ) = 0;
	virtual void ClearAttributeCache( CBaseEntity *pEntity ) = 0;
	virtual bool GetItemAttribute( CBaseEntity *pItem, int nDefIndex, float *pflValue ) = 0;
	virtual int GetItemAttributes( CBaseEntity *pItem, int *pnDefIndexes, float *pflValues, int nMax ) = 0;
	virtual int GetItemDefinitionAttributes( int nItemDefIndex, int nClass, int *pnDefIndexes, float *pflValues, int nMax ) = 0;
	virtual bool AddCustomPlayerAttribute( CBaseEntity *pPlayer, const char *pszName, float flValue, float flDuration ) = 0;
	virtual bool RemoveCustomPlayerAttribute( CBaseEntity *pPlayer, const char *pszName ) = 0;
};

#endif
