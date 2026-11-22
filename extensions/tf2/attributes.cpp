/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Team Fortress 2 Extension
 * Item attribute natives for TF2 Rebalanced
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
#include "util.h"
#include "attributes.h"
#include "iktbmattributes.h"

#include <string>
#include <vector>

static IKTBMAttributes *g_pAttributes = NULL;

#define MAX_LISTED_ATTRIBUTES	1024

// Names of attributes, for error messages.
#define MAX_ATTRIBUTE_NAME		128

bool FindAttributeInterface(ISmmAPI *ismm)
{
	CreateInterfaceFn factory = ismm->GetServerFactory(false);
	g_pAttributes = factory ? (IKTBMAttributes *)factory(KTBM_ATTRIBUTES_INTERFACE_VERSION, NULL) : NULL;

	return g_pAttributes != NULL;
}

bool HasAttributeInterface()
{
	return g_pAttributes != NULL;
}

#define ATTRIBUTES_OR_THROW() \
	if (!g_pAttributes) \
	{ \
		return pContext->ThrowNativeError("This build of the game has no attribute interface (%s)", KTBM_ATTRIBUTES_INTERFACE_VERSION); \
	}

static cell_t ThrowInvalidEntity(IPluginContext *pContext, cell_t ref)
{
	return pContext->ThrowNativeError("Entity %d (%d) is invalid", gamehelpers->ReferenceToIndex(ref), ref);
}

/**
 * The definition index of the attribute whose name a native was given.
 * Throws and returns false if there is none: a name that doesn't exist is a
 * mistake in the plugin, or an attribute this server's item schema lacks.
 */
static bool FindAttributeByName(IPluginContext *pContext, cell_t nameAddr, int *pDefIndex)
{
	char *name;
	pContext->LocalToString(nameAddr, &name);

	*pDefIndex = g_pAttributes->GetAttributeDefIndex(name);
	if (*pDefIndex < 0)
	{
		pContext->ThrowNativeError("Attribute \"%s\" does not exist", name);
		return false;
	}

	return true;
}

/**
 * Whether the attribute can be set on the entity. Throws and returns false
 * if not, with an error that says which of the two is the matter.
 */
static bool CanSetAttribute(IPluginContext *pContext, CBaseEntity *pEntity, cell_t ref, int defIndex)
{
	if (!g_pAttributes->HasAttributes(pEntity))
	{
		pContext->ThrowNativeError("Entity %d (%d) can't have attributes: it is not a player, a weapon or a wearable",
			gamehelpers->ReferenceToIndex(ref), ref);
		return false;
	}

	if (!g_pAttributes->IsAttributeNumber(defIndex))
	{
		char name[MAX_ATTRIBUTE_NAME];
		if (g_pAttributes->GetAttributeName(defIndex, name, (int)sizeof(name)))
		{
			pContext->ThrowNativeError("Attribute %d (\"%s\") can't be set: its value is not a number", defIndex, name);
		}
		else
		{
			pContext->ThrowNativeError("Attribute %d does not exist", defIndex);
		}
		return false;
	}

	return true;
}

/**
 * Fills a plugin's two arrays with the attributes that `fill` lists, and
 * returns what it returned: how many there are in all, or -1.
 *
 * The game fills arrays of ints and floats. A plugin's arrays are cells, so
 * the values go through buffers of the game's types on the way.
 */
template <typename Fill>
static cell_t ListAttributes(IPluginContext *pContext, cell_t indexesAddr, cell_t valuesAddr, cell_t maxlength, Fill fill)
{
	if (maxlength < 0)
	{
		return pContext->ThrowNativeError("Array size %d is invalid", maxlength);
	}

	int size = (maxlength < MAX_LISTED_ATTRIBUTES) ? maxlength : MAX_LISTED_ATTRIBUTES;
	std::vector<int> indexes(size);
	std::vector<float> values(size);

	int count = fill(size ? &indexes[0] : NULL, size ? &values[0] : NULL, size);

	if (count > 0 && size > 0)
	{
		cell_t *pIndexes, *pValues;
		pContext->LocalToPhysAddr(indexesAddr, &pIndexes);
		pContext->LocalToPhysAddr(valuesAddr, &pValues);

		for (int i = 0; i < count && i < size; i++)
		{
			pIndexes[i] = indexes[i];
			pValues[i] = sp_ftoc(values[i]);
		}
	}

	return count;
}

// native bool TF2R_AttributesAvailable();
static cell_t Native_AttributesAvailable(IPluginContext *pContext, const cell_t *params)
{
	return g_pAttributes ? 1 : 0;
}

// native int TF2R_GetAttributeDefIndex(const char[] name);
static cell_t Native_GetAttributeDefIndex(IPluginContext *pContext, const cell_t *params)
{
	ATTRIBUTES_OR_THROW();

	char *name;
	pContext->LocalToString(params[1], &name);

	return g_pAttributes->GetAttributeDefIndex(name);
}

typedef bool (IKTBMAttributes::*AttributeTextFn)(int, char *, int);

static cell_t GetAttributeText(IPluginContext *pContext, const cell_t *params, AttributeTextFn fn)
{
	if (params[3] <= 0)
	{
		return pContext->ThrowNativeError("Buffer size %d is invalid", params[3]);
	}

	char *buffer;
	pContext->LocalToString(params[2], &buffer);
	buffer[0] = '\0';

	return (g_pAttributes->*fn)(params[1], buffer, params[3]) ? 1 : 0;
}

// native bool TF2R_GetAttributeName(int defIndex, char[] name, int maxlength);
static cell_t Native_GetAttributeName(IPluginContext *pContext, const cell_t *params)
{
	ATTRIBUTES_OR_THROW();

	return GetAttributeText(pContext, params, &IKTBMAttributes::GetAttributeName);
}

// native bool TF2R_GetAttributeClass(int defIndex, char[] attribClass, int maxlength);
static cell_t Native_GetAttributeClass(IPluginContext *pContext, const cell_t *params)
{
	ATTRIBUTES_OR_THROW();

	return GetAttributeText(pContext, params, &IKTBMAttributes::GetAttributeClass);
}

// native bool TF2R_IsAttributeNumber(int defIndex);
static cell_t Native_IsAttributeNumber(IPluginContext *pContext, const cell_t *params)
{
	ATTRIBUTES_OR_THROW();

	return g_pAttributes->IsAttributeNumber(params[1]) ? 1 : 0;
}

// native bool TF2R_IsAttributeInteger(int defIndex);
static cell_t Native_IsAttributeInteger(IPluginContext *pContext, const cell_t *params)
{
	ATTRIBUTES_OR_THROW();

	return g_pAttributes->IsAttributeStoredAsInteger(params[1]) ? 1 : 0;
}

// native float TF2R_HookValueFloat(int entity, const char[] attribClass, float base);
static cell_t Native_HookValueFloat(IPluginContext *pContext, const cell_t *params)
{
	ATTRIBUTES_OR_THROW();

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return ThrowInvalidEntity(pContext, params[1]);
	}

	char *attribClass;
	pContext->LocalToString(params[2], &attribClass);

	return sp_ftoc(g_pAttributes->HookValueFloat(pEntity, attribClass, sp_ctof(params[3])));
}

// native int TF2R_HookValueInt(int entity, const char[] attribClass, int base);
static cell_t Native_HookValueInt(IPluginContext *pContext, const cell_t *params)
{
	ATTRIBUTES_OR_THROW();

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return ThrowInvalidEntity(pContext, params[1]);
	}

	char *attribClass;
	pContext->LocalToString(params[2], &attribClass);

	return g_pAttributes->HookValueInt(pEntity, attribClass, params[3]);
}

// native bool TF2R_HookValueString(int entity, const char[] attribClass, const char[] base, char[] value, int maxlength);
static cell_t Native_HookValueString(IPluginContext *pContext, const cell_t *params)
{
	ATTRIBUTES_OR_THROW();

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return ThrowInvalidEntity(pContext, params[1]);
	}

	if (params[5] <= 0)
	{
		return pContext->ThrowNativeError("Buffer size %d is invalid", params[5]);
	}

	char *attribClass, *base, *value;
	pContext->LocalToString(params[2], &attribClass);
	pContext->LocalToString(params[3], &base);
	pContext->LocalToString(params[4], &value);

	std::string classCopy(attribClass);
	std::string baseCopy(base);

	return g_pAttributes->HookValueString(pEntity, classCopy.c_str(), baseCopy.c_str(), value, params[5]) ? 1 : 0;
}

// native bool TF2R_HasAttributes(int entity);
static cell_t Native_HasAttributes(IPluginContext *pContext, const cell_t *params)
{
	ATTRIBUTES_OR_THROW();

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return ThrowInvalidEntity(pContext, params[1]);
	}

	return g_pAttributes->HasAttributes(pEntity) ? 1 : 0;
}

static cell_t SetEntityAttribute(IPluginContext *pContext, cell_t ref, int defIndex, cell_t value)
{
	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(ref);
	if (!pEntity)
	{
		return ThrowInvalidEntity(pContext, ref);
	}

	if (!CanSetAttribute(pContext, pEntity, ref, defIndex))
	{
		return 0;
	}

	if (!g_pAttributes->SetRuntimeAttribute(pEntity, defIndex, sp_ctof(value)))
	{
		return pContext->ThrowNativeError("Could not set attribute %d on entity %d", defIndex, gamehelpers->ReferenceToIndex(ref));
	}

	return 1;
}

// native void TF2R_SetAttribute(int entity, int defIndex, float value);
static cell_t Native_SetAttribute(IPluginContext *pContext, const cell_t *params)
{
	ATTRIBUTES_OR_THROW();

	return SetEntityAttribute(pContext, params[1], params[2], params[3]);
}

// native void TF2R_SetAttributeByName(int entity, const char[] name, float value);
static cell_t Native_SetAttributeByName(IPluginContext *pContext, const cell_t *params)
{
	ATTRIBUTES_OR_THROW();

	int defIndex;
	if (!FindAttributeByName(pContext, params[2], &defIndex))
	{
		return 0;
	}

	return SetEntityAttribute(pContext, params[1], defIndex, params[3]);
}

static cell_t RemoveEntityAttribute(IPluginContext *pContext, cell_t ref, int defIndex)
{
	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(ref);
	if (!pEntity)
	{
		return ThrowInvalidEntity(pContext, ref);
	}

	return g_pAttributes->RemoveRuntimeAttribute(pEntity, defIndex) ? 1 : 0;
}

// native bool TF2R_RemoveAttribute(int entity, int defIndex);
static cell_t Native_RemoveAttribute(IPluginContext *pContext, const cell_t *params)
{
	ATTRIBUTES_OR_THROW();

	return RemoveEntityAttribute(pContext, params[1], params[2]);
}

// native bool TF2R_RemoveAttributeByName(int entity, const char[] name);
static cell_t Native_RemoveAttributeByName(IPluginContext *pContext, const cell_t *params)
{
	ATTRIBUTES_OR_THROW();

	int defIndex;
	if (!FindAttributeByName(pContext, params[2], &defIndex))
	{
		return 0;
	}

	return RemoveEntityAttribute(pContext, params[1], defIndex);
}

static cell_t GetEntityAttribute(IPluginContext *pContext, cell_t ref, int defIndex, cell_t valueAddr)
{
	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(ref);
	if (!pEntity)
	{
		return ThrowInvalidEntity(pContext, ref);
	}

	float value;
	if (!g_pAttributes->GetRuntimeAttribute(pEntity, defIndex, &value))
	{
		return 0;
	}

	cell_t *pValue;
	pContext->LocalToPhysAddr(valueAddr, &pValue);
	*pValue = sp_ftoc(value);

	return 1;
}

// native bool TF2R_GetAttribute(int entity, int defIndex, float &value);
static cell_t Native_GetAttribute(IPluginContext *pContext, const cell_t *params)
{
	ATTRIBUTES_OR_THROW();

	return GetEntityAttribute(pContext, params[1], params[2], params[3]);
}

// native bool TF2R_GetAttributeByName(int entity, const char[] name, float &value);
static cell_t Native_GetAttributeByName(IPluginContext *pContext, const cell_t *params)
{
	ATTRIBUTES_OR_THROW();

	int defIndex;
	if (!FindAttributeByName(pContext, params[2], &defIndex))
	{
		return 0;
	}

	return GetEntityAttribute(pContext, params[1], defIndex, params[3]);
}

// native int TF2R_GetAttributes(int entity, int[] defIndexes, float[] values, int maxlength);
static cell_t Native_GetAttributes(IPluginContext *pContext, const cell_t *params)
{
	ATTRIBUTES_OR_THROW();

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return ThrowInvalidEntity(pContext, params[1]);
	}

	return ListAttributes(pContext, params[2], params[3], params[4],
		[pEntity](int *indexes, float *values, int max) -> int
		{
			return g_pAttributes->GetRuntimeAttributes(pEntity, indexes, values, max);
		});
}

// native void TF2R_RemoveAllAttributes(int entity);
static cell_t Native_RemoveAllAttributes(IPluginContext *pContext, const cell_t *params)
{
	ATTRIBUTES_OR_THROW();

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return ThrowInvalidEntity(pContext, params[1]);
	}

	g_pAttributes->RemoveAllRuntimeAttributes(pEntity);
	return 1;
}

// native void TF2R_ClearAttributeCache(int entity);
static cell_t Native_ClearAttributeCache(IPluginContext *pContext, const cell_t *params)
{
	ATTRIBUTES_OR_THROW();

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return ThrowInvalidEntity(pContext, params[1]);
	}

	g_pAttributes->ClearAttributeCache(pEntity);
	return 1;
}

// native bool TF2R_GetItemAttribute(int entity, int defIndex, float &value);
static cell_t Native_GetItemAttribute(IPluginContext *pContext, const cell_t *params)
{
	ATTRIBUTES_OR_THROW();

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return ThrowInvalidEntity(pContext, params[1]);
	}

	float value;
	if (!g_pAttributes->GetItemAttribute(pEntity, params[2], &value))
	{
		return 0;
	}

	cell_t *pValue;
	pContext->LocalToPhysAddr(params[3], &pValue);
	*pValue = sp_ftoc(value);

	return 1;
}

// native int TF2R_GetItemAttributes(int entity, int[] defIndexes, float[] values, int maxlength);
static cell_t Native_GetItemAttributes(IPluginContext *pContext, const cell_t *params)
{
	ATTRIBUTES_OR_THROW();

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if (!pEntity)
	{
		return ThrowInvalidEntity(pContext, params[1]);
	}

	return ListAttributes(pContext, params[2], params[3], params[4],
		[pEntity](int *indexes, float *values, int max) -> int
		{
			return g_pAttributes->GetItemAttributes(pEntity, indexes, values, max);
		});
}

// native int TF2R_GetItemDefinitionAttributes(int itemDefIndex, TFClassType classType, int[] defIndexes, float[] values, int maxlength);
static cell_t Native_GetItemDefinitionAttributes(IPluginContext *pContext, const cell_t *params)
{
	ATTRIBUTES_OR_THROW();

	int itemDefIndex = params[1];
	int classType = params[2];

	return ListAttributes(pContext, params[3], params[4], params[5],
		[itemDefIndex, classType](int *indexes, float *values, int max) -> int
		{
			return g_pAttributes->GetItemDefinitionAttributes(itemDefIndex, classType, indexes, values, max);
		});
}

/**
 * The player and the attribute a timed attribute native was given. Throws
 * and returns false if either isn't one.
 */
static bool FindPlayerAndAttribute(IPluginContext *pContext, const cell_t *params, CBaseEntity **ppPlayer, char **pName)
{
	*ppPlayer = UTIL_GetCBaseEntity(params[1], true);
	if (!*ppPlayer)
	{
		pContext->ThrowNativeError("Client index %d is not valid", params[1]);
		return false;
	}

	int defIndex;
	if (!FindAttributeByName(pContext, params[2], &defIndex))
	{
		return false;
	}

	pContext->LocalToString(params[2], pName);
	if (!g_pAttributes->IsAttributeNumber(defIndex))
	{
		pContext->ThrowNativeError("Attribute \"%s\" can't be set: its value is not a number", *pName);
		return false;
	}

	return true;
}

// native void TF2R_AddCustomPlayerAttribute(int client, const char[] name, float value, float duration = -1.0);
static cell_t Native_AddCustomPlayerAttribute(IPluginContext *pContext, const cell_t *params)
{
	ATTRIBUTES_OR_THROW();

	CBaseEntity *pPlayer;
	char *name;
	if (!FindPlayerAndAttribute(pContext, params, &pPlayer, &name))
	{
		return 0;
	}

	if (!g_pAttributes->AddCustomPlayerAttribute(pPlayer, name, sp_ctof(params[3]), sp_ctof(params[4])))
	{
		return pContext->ThrowNativeError("Could not add attribute \"%s\" to client %d", name, params[1]);
	}

	return 1;
}

// native void TF2R_RemoveCustomPlayerAttribute(int client, const char[] name);
static cell_t Native_RemoveCustomPlayerAttribute(IPluginContext *pContext, const cell_t *params)
{
	ATTRIBUTES_OR_THROW();

	CBaseEntity *pPlayer;
	char *name;
	if (!FindPlayerAndAttribute(pContext, params, &pPlayer, &name))
	{
		return 0;
	}

	g_pAttributes->RemoveCustomPlayerAttribute(pPlayer, name);
	return 1;
}

sp_nativeinfo_t g_TFAttributeNatives[] =
{
	{"TF2R_AttributesAvailable",			Native_AttributesAvailable},
	{"TF2R_GetAttributeDefIndex",			Native_GetAttributeDefIndex},
	{"TF2R_GetAttributeName",				Native_GetAttributeName},
	{"TF2R_GetAttributeClass",				Native_GetAttributeClass},
	{"TF2R_IsAttributeNumber",				Native_IsAttributeNumber},
	{"TF2R_IsAttributeInteger",				Native_IsAttributeInteger},
	{"TF2R_HookValueFloat",					Native_HookValueFloat},
	{"TF2R_HookValueInt",					Native_HookValueInt},
	{"TF2R_HookValueString",				Native_HookValueString},
	{"TF2R_HasAttributes",					Native_HasAttributes},
	{"TF2R_SetAttribute",					Native_SetAttribute},
	{"TF2R_SetAttributeByName",				Native_SetAttributeByName},
	{"TF2R_RemoveAttribute",				Native_RemoveAttribute},
	{"TF2R_RemoveAttributeByName",			Native_RemoveAttributeByName},
	{"TF2R_GetAttribute",					Native_GetAttribute},
	{"TF2R_GetAttributeByName",				Native_GetAttributeByName},
	{"TF2R_GetAttributes",					Native_GetAttributes},
	{"TF2R_RemoveAllAttributes",			Native_RemoveAllAttributes},
	{"TF2R_ClearAttributeCache",			Native_ClearAttributeCache},
	{"TF2R_GetItemAttribute",				Native_GetItemAttribute},
	{"TF2R_GetItemAttributes",				Native_GetItemAttributes},
	{"TF2R_GetItemDefinitionAttributes",	Native_GetItemDefinitionAttributes},
	{"TF2R_AddCustomPlayerAttribute",		Native_AddCustomPlayerAttribute},
	{"TF2R_RemoveCustomPlayerAttribute",	Native_RemoveCustomPlayerAttribute},
	{NULL,									NULL}
};
