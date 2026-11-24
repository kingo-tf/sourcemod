/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Team Fortress 2 Extension
 * Natives for the functions TF2 Rebalanced exposes by name
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 */

#include "extension.h"
#include "exposed.h"
#include <iktbmgamedata.h>

static IKTBMGameData *g_pExposed = NULL;

// Names of what the game exposes, for TF2R_GetExposed
#define MAX_EXPOSED_NAME	256

bool FindExposedInterface(ISmmAPI *ismm)
{
	CreateInterfaceFn factory = ismm->GetServerFactory(false);
	g_pExposed = factory ? (IKTBMGameData *)factory(KTBM_GAMEDATA_INTERFACE_VERSION, NULL) : NULL;

	return g_pExposed != NULL;
}

bool HasExposedInterface()
{
	return g_pExposed != NULL;
}

#define EXPOSED_OR_THROW() \
	if (!g_pExposed) \
	{ \
		return pContext->ThrowNativeError("This build of the game exposes nothing by name (%s)", KTBM_GAMEDATA_INTERFACE_VERSION); \
	}

/**
 * Whether the plugin was compiled with an Address of 64 bits. Such a plugin
 * gets one from a native in a buffer that is the native's first parameter,
 * ahead of the ones its declaration has.
 */
static bool HasInt64Address(IPluginContext *pContext)
{
	return pContext->GetRuntime()->FindPubvarByName("__Int64_Address__", nullptr) == SP_ERROR_NONE;
}

static cell_t ReturnAddress(IPluginContext *pContext, const cell_t *params, bool int64Address, void *addr)
{
	if (int64Address)
	{
		cell_t *buffer;
		if (int err = pContext->LocalToPhysAddr(params[1], &buffer); err != SP_ERROR_NONE)
		{
			return pContext->ThrowNativeErrorEx(err, "Could not read argument");
		}
		*reinterpret_cast<int64_t *>(buffer) = reinterpret_cast<uintptr_t>(addr);
	}

	return static_cast<cell_t>(reinterpret_cast<uintptr_t>(addr));
}

// native bool TF2R_ExposedAvailable();
static cell_t Native_ExposedAvailable(IPluginContext *pContext, const cell_t *params)
{
	return g_pExposed != NULL;
}

// native Address TF2R_GetFunctionAddress(const char[] name);
static cell_t Native_GetFunctionAddress(IPluginContext *pContext, const cell_t *params)
{
	EXPOSED_OR_THROW();

	const bool int64Address = HasInt64Address(pContext);

	char *name;
	pContext->LocalToString(params[int64Address ? 2 : 1], &name);

	return ReturnAddress(pContext, params, int64Address, g_pExposed->GetFunctionAddress(name));
}

// native Address TF2R_GetGlobalAddress(const char[] name);
static cell_t Native_GetGlobalAddress(IPluginContext *pContext, const cell_t *params)
{
	EXPOSED_OR_THROW();

	const bool int64Address = HasInt64Address(pContext);

	char *name;
	pContext->LocalToString(params[int64Address ? 2 : 1], &name);

	return ReturnAddress(pContext, params, int64Address, g_pExposed->GetAddress(name));
}

// native int TF2R_GetVTableOffset(const char[] name);
static cell_t Native_GetVTableOffset(IPluginContext *pContext, const cell_t *params)
{
	EXPOSED_OR_THROW();

	char *name;
	pContext->LocalToString(params[1], &name);

	return g_pExposed->GetVTableOffset(name);
}

// native int TF2R_GetMemberOffset(const char[] name);
static cell_t Native_GetMemberOffset(IPluginContext *pContext, const cell_t *params)
{
	EXPOSED_OR_THROW();

	char *name;
	pContext->LocalToString(params[1], &name);

	return g_pExposed->GetMemberOffset(name);
}

// native int TF2R_GetExposedCount();
static cell_t Native_GetExposedCount(IPluginContext *pContext, const cell_t *params)
{
	EXPOSED_OR_THROW();

	return g_pExposed->GetExposedCount();
}

// native bool TF2R_GetExposed(int index, char[] name, int maxlength, TF2RExposedKind &kind);
static cell_t Native_GetExposed(IPluginContext *pContext, const cell_t *params)
{
	EXPOSED_OR_THROW();

	char name[MAX_EXPOSED_NAME];
	int kind = 0;
	if (!g_pExposed->GetExposed(params[1], name, sizeof(name), &kind))
	{
		return 0;
	}

	pContext->StringToLocalUTF8(params[2], params[3], name, NULL);

	cell_t *kindAddr;
	pContext->LocalToPhysAddr(params[4], &kindAddr);
	*kindAddr = kind;

	return 1;
}

sp_nativeinfo_t g_TFExposedNatives[] =
{
	{"TF2R_ExposedAvailable",		Native_ExposedAvailable},
	{"TF2R_GetFunctionAddress",		Native_GetFunctionAddress},
	{"TF2R_GetGlobalAddress",		Native_GetGlobalAddress},
	{"TF2R_GetVTableOffset",		Native_GetVTableOffset},
	{"TF2R_GetMemberOffset",		Native_GetMemberOffset},
	{"TF2R_GetExposedCount",		Native_GetExposedCount},
	{"TF2R_GetExposed",				Native_GetExposed},
	{NULL,							NULL}
};
