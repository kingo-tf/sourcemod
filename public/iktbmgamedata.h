#ifndef IKTBMGAMEDATA_H
#define IKTBMGAMEDATA_H
#ifdef _WIN32
#pragma once
#endif

#define KTBM_GAMEDATA_INTERFACE_VERSION "KTBMGameData001"

#define KTBM_EXPOSED_FUNCTION	0
#define KTBM_EXPOSED_VIRTUAL	1
#define KTBM_EXPOSED_ADDRESS	2
#define KTBM_EXPOSED_MEMBER		3

class IKTBMGameData
{
public:

	virtual void *GetFunctionAddress( const char *pszName ) = 0;
	virtual int GetVTableOffset( const char *pszName ) = 0;
	virtual void *GetAddress( const char *pszName ) = 0;
	virtual int GetExposedCount() = 0;
	virtual bool GetExposed( int nIndex, char *pszName, int cchName, int *pnKind ) = 0;
	virtual int GetMemberOffset( const char *pszName ) = 0;
};

#endif
