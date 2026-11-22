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

#ifndef _INCLUDE_SOURCEMOD_TF2R_ATTRIBUTES_H_
#define _INCLUDE_SOURCEMOD_TF2R_ATTRIBUTES_H_

#include "extension.h"

/**
 * @brief Asks the game for its attribute interface (iktbmattributes.h). The
 * game publishes it by name, so this needs no gamedata.
 *
 * @param ismm		Metamod's API, as SDK_OnMetamodLoad() gets it.
 * @return			False if this build of the game has no such interface. The
 *					natives are registered all the same and throw when called.
 */
bool FindAttributeInterface(ISmmAPI *ismm);

/**
 * @brief Whether FindAttributeInterface() found the interface.
 */
bool HasAttributeInterface();

extern sp_nativeinfo_t g_TFAttributeNatives[];

#endif // _INCLUDE_SOURCEMOD_TF2R_ATTRIBUTES_H_
