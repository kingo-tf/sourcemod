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

#if defined _tf2r_attributes_included
 #endinput
#endif
#define _tf2r_attributes_included

#include <tf2>

#define TF2R_INVALID_ATTRIBUTE	(-1)	/**< Returned by TF2R_GetAttributeDefIndex if the attribute does not exist */

/**
 * Returns whether the attribute natives can be used on this server.
 * If they cannot, every other native in this file throws an error.
 *
 * @return              True if the natives are available, false otherwise.
 */
native bool TF2R_AttributesAvailable();

/**
 * Finds the definition index of an attribute by its name.
 *
 * @note Cache the result instead of calling this repeatedly, and look it up
 *       again after a map change: on servers that add their own attributes,
 *       an index can differ between maps.
 *
 * @param name          Attribute name, as in the "name" key in items_game.
 * @return              Attribute definition index, or TF2R_INVALID_ATTRIBUTE
 *                      if the attribute does not exist.
 * @error               No mod support.
 */
native int TF2R_GetAttributeDefIndex(const char[] name);

/**
 * Retrieves the name of an attribute.
 *
 * @param defIndex      Attribute definition index.
 * @param name          Buffer to store the name.
 * @param maxlength     Maximum length of the buffer.
 * @return              True on success, false if the attribute does not exist.
 * @error               Invalid buffer size, or no mod support.
 */
native bool TF2R_GetAttributeName(int defIndex, char[] name, int maxlength);

/**
 * Retrieves the class of an attribute. The TF2R_HookValue natives take an
 * attribute class.
 *
 * @param defIndex      Attribute definition index.
 * @param attribClass   Buffer to store the class, as in the "attribute_class"
 *                      key in items_game. Empty if the attribute has no class.
 * @param maxlength     Maximum length of the buffer.
 * @return              True on success, false if the attribute does not exist.
 * @error               Invalid buffer size, or no mod support.
 */
native bool TF2R_GetAttributeClass(int defIndex, char[] attribClass, int maxlength);

/**
 * Returns whether an attribute has a numeric value.
 * Only numeric attributes can be set, read or listed with these natives.
 * String attributes can only be read with TF2R_HookValueString.
 *
 * @param defIndex      Attribute definition index.
 * @return              True if the value is numeric, false if it is not or
 *                      the attribute does not exist.
 * @error               No mod support.
 */
native bool TF2R_IsAttributeNumber(int defIndex);

/**
 * Returns whether an attribute's value is stored as an integer instead of a
 * float. Use TF2R_SetAttributeInt and TF2R_GetAttributeInt for those.
 *
 * @param defIndex      Attribute definition index.
 * @return              True if the value is stored as an integer, false if it
 *                      is stored as a float or the attribute does not exist.
 * @error               No mod support.
 */
native bool TF2R_IsAttributeInteger(int defIndex);

/**
 * Applies the attributes of the given class to a value, the same way the
 * game does.
 *
 * @param entity        Entity index or reference. Checking a player also
 *                      checks their equipped items. Checking a weapon or
 *                      wearable also checks its owner.
 * @param attribClass   Attribute class, e.g. "mult_dmg".
 * @param base          Initial value. Usually 1.0 for a class that multiplies
 *                      and 0.0 for a class that adds.
 * @return              The resulting value, or the initial value if the
 *                      entity cannot have attributes.
 * @error               Invalid entity, or no mod support.
 */
native float TF2R_HookValueFloat(int entity, const char[] attribClass, float base);

/**
 * Applies the attributes of the given class to an integer value.
 * See TF2R_HookValueFloat.
 *
 * @param entity        Entity index or reference.
 * @param attribClass   Attribute class.
 * @param base          Initial value.
 * @return              The resulting value, or the initial value if the
 *                      entity cannot have attributes.
 * @error               Invalid entity, or no mod support.
 */
native int TF2R_HookValueInt(int entity, const char[] attribClass, int base);

/**
 * Retrieves the string value of the given attribute class on an entity.
 * See TF2R_HookValueFloat for which attributes are checked.
 *
 * @param entity        Entity index or reference.
 * @param attribClass   Attribute class.
 * @param base          Initial value. It is stored in the buffer unchanged if
 *                      no attribute of the class is found.
 * @param value         Buffer to store the result.
 * @param maxlength     Maximum length of the buffer.
 * @return              True if the entity can have attributes, false
 *                      otherwise. The buffer is filled in either case.
 * @error               Invalid entity or buffer size, or no mod support.
 */
native bool TF2R_HookValueString(int entity, const char[] attribClass, const char[] base, char[] value, int maxlength);

/**
 * Returns whether an entity can have attributes. Players, weapons and
 * wearables can.
 *
 * @param entity        Entity index or reference.
 * @return              True if the entity can have attributes, false otherwise.
 * @error               Invalid entity, or no mod support.
 */
native bool TF2R_HasAttributes(int entity);

/**
 * Sets a runtime attribute on an entity, adding it if it is not present.
 * On a weapon or wearable, a runtime attribute overrides the item's own
 * attribute of the same definition. The change takes effect immediately, and
 * a player's speed is recalculated.
 *
 * @param entity        Entity index or reference. Must be a player, weapon or
 *                      wearable.
 * @param defIndex      Attribute definition index.
 * @param value         Value to set. For attributes stored as integers, use
 *                      TF2R_SetAttributeInt.
 * @error               Invalid entity, entity cannot have attributes,
 *                      attribute does not exist or is not numeric, or no mod
 *                      support.
 */
native void TF2R_SetAttribute(int entity, int defIndex, float value);

/**
 * Sets a runtime attribute on an entity by attribute name.
 * See TF2R_SetAttribute.
 *
 * @param entity        Entity index or reference. Must be a player, weapon or
 *                      wearable.
 * @param name          Attribute name.
 * @param value         Value to set.
 * @error               Invalid entity, entity cannot have attributes,
 *                      attribute does not exist or is not numeric, or no mod
 *                      support.
 */
native void TF2R_SetAttributeByName(int entity, const char[] name, float value);

/**
 * Removes a runtime attribute from an entity. On a weapon or wearable, the
 * item's own attribute of the same definition applies again.
 *
 * @param entity        Entity index or reference.
 * @param defIndex      Attribute definition index.
 * @return              True if the attribute was removed, false if the entity
 *                      did not have it.
 * @error               Invalid entity, or no mod support.
 */
native bool TF2R_RemoveAttribute(int entity, int defIndex);

/**
 * Removes a runtime attribute from an entity by attribute name.
 * See TF2R_RemoveAttribute.
 *
 * @param entity        Entity index or reference.
 * @param name          Attribute name.
 * @return              True if the attribute was removed, false if the entity
 *                      did not have it.
 * @error               Invalid entity, attribute does not exist, or no mod
 *                      support.
 */
native bool TF2R_RemoveAttributeByName(int entity, const char[] name);

/**
 * Retrieves the value of a runtime attribute on an entity.
 *
 * @note This only reads attributes that were set at run time. Use
 *       TF2R_GetItemAttribute to also check the item's own attributes, or
 *       TF2R_HookValueFloat for the combined effect of all attributes.
 *
 * @param entity        Entity index or reference.
 * @param defIndex      Attribute definition index.
 * @param value         Variable to store the value. Not changed if the entity
 *                      does not have the attribute.
 * @return              True if the entity has the runtime attribute, false
 *                      otherwise.
 * @error               Invalid entity, or no mod support.
 */
native bool TF2R_GetAttribute(int entity, int defIndex, float &value);

/**
 * Retrieves the value of a runtime attribute on an entity by attribute name.
 * See TF2R_GetAttribute.
 *
 * @param entity        Entity index or reference.
 * @param name          Attribute name.
 * @param value         Variable to store the value. Not changed if the entity
 *                      does not have the attribute.
 * @return              True if the entity has the runtime attribute, false
 *                      otherwise.
 * @error               Invalid entity, attribute does not exist, or no mod
 *                      support.
 */
native bool TF2R_GetAttributeByName(int entity, const char[] name, float &value);

/**
 * Retrieves all runtime attributes on an entity. This includes runtime
 * attributes set by the game, such as upgrades.
 *
 * @param entity        Entity index or reference.
 * @param defIndexes    Array to store the attribute definition indexes.
 * @param values        Array to store the attribute values, in the same order.
 * @param maxlength     Size of both arrays. Pass 0 to only get the count.
 * @return              Number of runtime attributes on the entity, which can
 *                      be higher than maxlength. 0 if the entity cannot have
 *                      attributes.
 * @error               Invalid entity or array size, or no mod support.
 */
native int TF2R_GetAttributes(int entity, int[] defIndexes, float[] values, int maxlength);

/**
 * Removes all runtime attributes from an entity. This includes runtime
 * attributes set by the game, such as upgrades.
 *
 * @param entity        Entity index or reference.
 * @error               Invalid entity, or no mod support.
 */
native void TF2R_RemoveAllAttributes(int entity);

/**
 * Clears the cached attribute hook results of an entity.
 * The natives that change attributes already do this, so it is rarely needed.
 *
 * @param entity        Entity index or reference.
 * @error               Invalid entity, or no mod support.
 */
native void TF2R_ClearAttributeCache(int entity);

/**
 * Retrieves the value of an attribute on a weapon or wearable. Its runtime
 * attributes are checked first, then the attributes of its item. Attributes
 * from the owner or from other items are not included.
 *
 * @param entity        Entity index or reference of a weapon or wearable.
 * @param defIndex      Attribute definition index.
 * @param value         Variable to store the value. Not changed if the
 *                      attribute is not found.
 * @return              True if the attribute was found, false if it was not
 *                      or the entity is not a weapon or wearable.
 * @error               Invalid entity, or no mod support.
 */
native bool TF2R_GetItemAttribute(int entity, int defIndex, float &value);

/**
 * Retrieves all attributes on a weapon or wearable: its runtime attributes,
 * followed by the attributes of its item that no runtime attribute overrides.
 * Attributes from the owner or from other items are not included.
 *
 * @note Only numeric attributes are returned. If the item has class-specific
 *       attributes, the ones for its owner's class are returned.
 *
 * @param entity        Entity index or reference of a weapon or wearable.
 * @param defIndexes    Array to store the attribute definition indexes.
 * @param values        Array to store the attribute values, in the same order.
 * @param maxlength     Size of both arrays. Pass 0 to only get the count.
 * @return              Number of attributes, which can be higher than
 *                      maxlength, or -1 if the entity is not a weapon or
 *                      wearable.
 * @error               Invalid entity or array size, or no mod support.
 */
native int TF2R_GetItemAttributes(int entity, int[] defIndexes, float[] values, int maxlength);

/**
 * Retrieves the attributes of an item definition from the item schema.
 * Only numeric attributes are returned.
 *
 * @param itemDefIndex  Item definition index, e.g. 13 for the Scattergun.
 * @param classType     Class to return class-specific attributes for. With
 *                      TFClass_Unknown, class-specific attributes are left
 *                      out.
 * @param defIndexes    Array to store the attribute definition indexes.
 * @param values        Array to store the attribute values, in the same order.
 * @param maxlength     Size of both arrays. Pass 0 to only get the count.
 * @return              Number of attributes, which can be higher than
 *                      maxlength, or -1 if the item definition does not
 *                      exist.
 * @error               Invalid array size, or no mod support.
 */
native int TF2R_GetItemDefinitionAttributes(int itemDefIndex, TFClassType classType, int[] defIndexes, float[] values, int maxlength);

/**
 * Adds a temporary attribute to a player. The game removes it when the
 * duration ends, and always when the player dies or spawns.
 *
 * @note This sets the player's runtime attribute of the same definition. It
 *       replaces a value set with TF2R_SetAttribute, and the attribute is
 *       removed when the temporary one ends.
 *
 * @param client        Client index.
 * @param name          Attribute name.
 * @param value         Value to set.
 * @param duration      Duration in seconds. With 0.0 or less, the attribute
 *                      lasts until the player dies or spawns.
 * @error               Invalid client index, attribute does not exist or is
 *                      not numeric, or no mod support.
 */
native void TF2R_AddCustomPlayerAttribute(int client, const char[] name, float value, float duration = -1.0);

/**
 * Removes a temporary attribute that was added to a player with
 * TF2R_AddCustomPlayerAttribute. Attributes set in any other way are not
 * affected.
 *
 * @param client        Client index.
 * @param name          Attribute name.
 * @error               Invalid client index, attribute does not exist or is
 *                      not numeric, or no mod support.
 */
native void TF2R_RemoveCustomPlayerAttribute(int client, const char[] name);

/**
 * Sets a runtime attribute whose value is stored as an integer.
 * See TF2R_IsAttributeInteger and TF2R_SetAttribute.
 *
 * @param entity        Entity index or reference. Must be a player, weapon or
 *                      wearable.
 * @param defIndex      Attribute definition index.
 * @param value         Value to set.
 * @error               Invalid entity, entity cannot have attributes,
 *                      attribute does not exist or is not numeric, or no mod
 *                      support.
 */
stock void TF2R_SetAttributeInt(int entity, int defIndex, int value)
{
	TF2R_SetAttribute(entity, defIndex, view_as<float>(value));
}

/**
 * Retrieves the value of a runtime attribute whose value is stored as an
 * integer. See TF2R_IsAttributeInteger and TF2R_GetAttribute.
 *
 * @param entity        Entity index or reference.
 * @param defIndex      Attribute definition index.
 * @param value         Variable to store the value. Not changed if the entity
 *                      does not have the attribute.
 * @return              True if the entity has the runtime attribute, false
 *                      otherwise.
 * @error               Invalid entity, or no mod support.
 */
stock bool TF2R_GetAttributeInt(int entity, int defIndex, int &value)
{
	float bits;
	if (!TF2R_GetAttribute(entity, defIndex, bits))
	{
		return false;
	}

	value = view_as<int>(bits);
	return true;
}

/**
 * Do not edit below this line!
 */
public Extension __ext_tf2r_attributes =
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
public void __ext_tf2r_attributes_SetNTVOptional()
{
	MarkNativeAsOptional("TF2R_AttributesAvailable");
	MarkNativeAsOptional("TF2R_GetAttributeDefIndex");
	MarkNativeAsOptional("TF2R_GetAttributeName");
	MarkNativeAsOptional("TF2R_GetAttributeClass");
	MarkNativeAsOptional("TF2R_IsAttributeNumber");
	MarkNativeAsOptional("TF2R_IsAttributeInteger");
	MarkNativeAsOptional("TF2R_HookValueFloat");
	MarkNativeAsOptional("TF2R_HookValueInt");
	MarkNativeAsOptional("TF2R_HookValueString");
	MarkNativeAsOptional("TF2R_HasAttributes");
	MarkNativeAsOptional("TF2R_SetAttribute");
	MarkNativeAsOptional("TF2R_SetAttributeByName");
	MarkNativeAsOptional("TF2R_RemoveAttribute");
	MarkNativeAsOptional("TF2R_RemoveAttributeByName");
	MarkNativeAsOptional("TF2R_GetAttribute");
	MarkNativeAsOptional("TF2R_GetAttributeByName");
	MarkNativeAsOptional("TF2R_GetAttributes");
	MarkNativeAsOptional("TF2R_RemoveAllAttributes");
	MarkNativeAsOptional("TF2R_ClearAttributeCache");
	MarkNativeAsOptional("TF2R_GetItemAttribute");
	MarkNativeAsOptional("TF2R_GetItemAttributes");
	MarkNativeAsOptional("TF2R_GetItemDefinitionAttributes");
	MarkNativeAsOptional("TF2R_AddCustomPlayerAttribute");
	MarkNativeAsOptional("TF2R_RemoveCustomPlayerAttribute");
}
#endif
