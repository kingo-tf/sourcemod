/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This file is part of the SourceMod/SourcePawn SDK.
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
 *
 * Version: $Id$
 */

#if defined _tf2r_exposed_included
 #endinput
#endif
#define _tf2r_exposed_included

/**
 * TF2 Rebalanced exposes some of its own functions by name, such as
 * "CTFPlayer::Regenerate". These natives give the address or the vtable offset
 * of one, as the running build of the game has it, so a plugin needs no
 * gamedata file for it and keeps working when the game updates:
 *
 *     DynamicDetour detour = new DynamicDetour(
 *         TF2R_GetFunctionAddress("CTFPlayer::Regenerate"),
 *         CallConv_THISCALL, ReturnType_Void, ThisPointer_CBaseEntity);
 *     detour.AddParam(HookParamType_Bool);
 *
 *     DynamicHook hook = new DynamicHook(
 *         TF2R_GetVTableOffset("CBasePlayer::ForceRespawn"),
 *         HookType_Entity, ReturnType_Void, ThisPointer_CBaseEntity);
 *
 *     StartPrepSDKCall(SDKCall_Player);
 *     PrepSDKCall_SetAddress(TF2R_GetFunctionAddress("CTFPlayer::Regenerate"));
 *
 * The parameters and the return value of a function are still the plugin's to
 * describe. Which names there are is up to the game: "ktbm_gamedata_list" in
 * the server console lists them, as does TF2R_GetExposed.
 *
 * A gamedata file can refer to the same names with an "exposed" key, in a
 * signature (a function or a global variable) or an offset (a virtual function
 * or a member variable). What the game answers is used on every platform; a
 * signature or an offset next to it is only what is left for a server whose
 * game doesn't know the name:
 *
 *     "Signatures" { "Regenerate"   { "library" "server"  "exposed" "CTFPlayer::Regenerate" } }
 *     "Offsets"    { "ForceRespawn" { "exposed" "CBasePlayer::ForceRespawn"  "windows64" "343"  "linux64" "345" } }
 *
 * An address entry can start from an exposed name as well, in place of a
 * signature; "read" and "offset" steps apply to it as usual. For a member
 * variable's name the address is that of a 32-bit number holding the member's
 * offset, which is what a plugin wants that reads an offset from memory:
 *
 *     "Addresses"  { "offsetof(CTFPlayer::m_flRespawnTimeOverride)" { "exposed" "CTFPlayer::m_flRespawnTimeOverride" } }
 *
 * The steps of an address entry apply to whichever it starts from. A
 * "signature" next to "exposed" is therefore only of use if it needs the same
 * steps; one that needs others can't share the entry.
 *
 * An entry that names something the game doesn't expose, and that has no
 * other value to go by, is reported in SourceMod's error log.
 *
 * The address of a function is safe to detour: the game only gives one for a
 * function that every call goes through. It gives none for a function its
 * code exposes without that guarantee.
 */

/**
 * What a name the game exposes stands for.
 */
enum TF2RExposedKind
{
	TF2RExposed_Function = 0,	/**< A function with an address: TF2R_GetFunctionAddress */
	TF2RExposed_Virtual = 1,	/**< A virtual function: TF2R_GetVTableOffset */
	TF2RExposed_Global = 2,		/**< A global variable or object: TF2R_GetGlobalAddress */
	TF2RExposed_Member = 3		/**< A member variable of a class: TF2R_GetMemberOffset */
};

/**
 * Returns whether the game exposes anything by name on this server.
 * If it does not, every other native in this file throws an error.
 *
 * @return              True if the natives are available, false otherwise.
 */
native bool TF2R_ExposedAvailable();

/**
 * Returns where a function the game exposes starts, to call it or to detour it.
 * A detour at this address sees every call of the function.
 *
 * A virtual function has no address here: it has an implementation per class,
 * so it is hooked or called through TF2R_GetVTableOffset.
 *
 * @param name          Name of the function, such as "CTFPlayer::Regenerate".
 *                      Case sensitive.
 * @return              Address of the function, or Address_Null if the game
 *                      exposes no function by that name.
 * @error               The game exposes nothing by name.
 */
native Address TF2R_GetFunctionAddress(const char[] name);

/**
 * Returns the index of a virtual function the game exposes in the vtable of its
 * class, which is what a virtual hook or a virtual SDKCall takes as its offset.
 *
 * @param name          Name of the function, such as "CBasePlayer::ForceRespawn".
 *                      Case sensitive.
 * @return              Offset of the function, or -1 if the game exposes no
 *                      virtual function by that name.
 * @error               The game exposes nothing by name.
 */
native int TF2R_GetVTableOffset(const char[] name);

/**
 * Returns the address of a global variable or object the game exposes. For a
 * global pointer this is where the pointer is kept, not what it points at.
 *
 * Given the name of a member variable, this returns the address of a 32-bit
 * number that holds the member's offset. TF2R_GetMemberOffset gives the offset
 * itself.
 *
 * @param name          Name of the variable. Case sensitive.
 * @return              Address of the variable, or Address_Null if the game
 *                      exposes none by that name.
 * @error               The game exposes nothing by name.
 */
native Address TF2R_GetGlobalAddress(const char[] name);

/**
 * Returns how far into an object of its class a member variable the game
 * exposes is, in bytes.
 *
 * @param name          Name of the member variable, such as
 *                      "CGlobalEntityList::m_entityListeners". Case sensitive.
 * @return              Offset of the member variable, or -1 if the game exposes
 *                      no member variable by that name.
 * @error               The game exposes nothing by name.
 */
native int TF2R_GetMemberOffset(const char[] name);

/**
 * Returns how many names the game exposes.
 *
 * @return              Number of names.
 * @error               The game exposes nothing by name.
 */
native int TF2R_GetExposedCount();

/**
 * Retrieves one of the names the game exposes.
 *
 * @param index         Index of the name, from 0 to TF2R_GetExposedCount() - 1.
 * @param name          Buffer to store the name in.
 * @param maxlength     Maximum length of the buffer.
 * @param kind          What the name stands for.
 * @return              True on success, false if there is no such index.
 * @error               The game exposes nothing by name.
 */
native bool TF2R_GetExposed(int index, char[] name, int maxlength, TF2RExposedKind &kind);

/**
 * Do not edit below this line!
 */
public Extension __ext_tf2r_exposed =
{
	name = "TF2 Tools",
	file = "game.tf2r.ext",
	autoload = 0,
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};

#if !defined REQUIRE_EXTENSIONS
public void __ext_tf2r_exposed_SetNTVOptional()
{
	MarkNativeAsOptional("TF2R_ExposedAvailable");
	MarkNativeAsOptional("TF2R_GetFunctionAddress");
	MarkNativeAsOptional("TF2R_GetVTableOffset");
	MarkNativeAsOptional("TF2R_GetGlobalAddress");
	MarkNativeAsOptional("TF2R_GetMemberOffset");
	MarkNativeAsOptional("TF2R_GetExposedCount");
	MarkNativeAsOptional("TF2R_GetExposed");
}
#endif
