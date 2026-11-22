/**
 * Checks the TF2R_ attribute natives (tf2r_attributes.inc) on a running
 * server.
 *
 *     sm_tf2r_attributes_test
 *
 * from the server's console. It needs a living player who holds a weapon, and
 * a bot will do:
 *
 *     sv_cheats 1; bot -team red -class scout
 *
 * Every check prints "ok" or "WRONG", and the last line says how many there
 * were. Each attribute the test sets is removed again.
 *
 * sm_tf2r_attributes_errors makes the calls the natives have to refuse (an
 * invalid entity, a name that isn't an attribute, an attribute whose value
 * isn't a number), one per frame because an error ends the function it is in.
 * Each has to show up in SourceMod's error log.
 */

#include <sourcemod>
#include <sdktools>
#include <tf2r_attributes>

#pragma semicolon 1
#pragma newdecls required

public Plugin myinfo =
{
	name = "TF2R Attributes Test",
	author = "TF2 Rebalanced",
	description = "Checks the TF2R_ attribute natives",
	version = "1.0",
	url = ""
};

#define ERROR_CALLS 9

int g_Right;
int g_Wrong;

int g_ErrorCall;
int g_ErrorPlayer;
int g_ErrorWeapon;

public void OnPluginStart()
{
	RegAdminCmd("sm_tf2r_attributes_test", Command_Test, ADMFLAG_ROOT,
		"Checks the TF2R_ attribute natives on a living player who holds a weapon");
	RegAdminCmd("sm_tf2r_attributes_errors", Command_Errors, ADMFLAG_ROOT,
		"Makes the calls the TF2R_ attribute natives have to refuse; each ends up in the error log");
}

void Check(bool right, const char[] what)
{
	if (right)
	{
		g_Right++;
		PrintToServer("ok     %s", what);
	}
	else
	{
		g_Wrong++;
		PrintToServer("WRONG  %s", what);
	}
}

bool Near(float a, float b)
{
	float scale = FloatAbs(b);
	if (scale < 1.0)
	{
		scale = 1.0;
	}

	return FloatAbs(a - b) <= 0.001 * scale;
}

public Action Command_Test(int client, int args)
{
	g_Right = 0;
	g_Wrong = 0;

	Check(TF2R_AttributesAvailable(), "the game provides attributes to plugins");
	if (!TF2R_AttributesAvailable())
	{
		return Plugin_Handled;
	}

	// Definitions
	char text[128];
	int damage = TF2R_GetAttributeDefIndex("damage bonus");
	int speed = TF2R_GetAttributeDefIndex("move speed bonus");
	Check(damage >= 0 && speed >= 0, "\"damage bonus\" and \"move speed bonus\" have a definition index");
	Check(TF2R_GetAttributeDefIndex("no such attribute") == TF2R_INVALID_ATTRIBUTE, "a name that isn't one gives TF2R_INVALID_ATTRIBUTE");
	Check(TF2R_GetAttributeName(damage, text, sizeof(text)) && StrEqual(text, "damage bonus", false), "the index gives the name back");
	Check(TF2R_GetAttributeClass(damage, text, sizeof(text)) && StrEqual(text, "mult_dmg", false), "its class is mult_dmg");
	Check(!TF2R_GetAttributeName(-1, text, sizeof(text)) && text[0] == '\0' && !TF2R_GetAttributeClass(1000000, text, sizeof(text)),
		"an index that isn't one has no name and no class");
	Check(TF2R_IsAttributeNumber(damage) && !TF2R_IsAttributeInteger(damage) && !TF2R_IsAttributeNumber(-1),
		"\"damage bonus\" has a number for a value and isn't stored as an integer");

	char speedClass[128];
	TF2R_GetAttributeClass(speed, speedClass, sizeof(speedClass));

	int indexes[64];
	float values[64];
	float value = -1.0;

	// The world has no attributes
	Check(!TF2R_HasAttributes(0) && TF2R_HookValueFloat(0, "mult_dmg", 5.0) == 5.0 && TF2R_HookValueInt(0, "mult_dmg", 5) == 5,
		"the world has no attributes, and a hook on it gives the base value");
	Check(!TF2R_RemoveAttribute(0, damage) && !TF2R_GetAttribute(0, damage, value) && !TF2R_GetItemAttribute(0, damage, value) && value == -1.0,
		"nothing can be read or removed there");
	Check(TF2R_GetAttributes(0, indexes, values, 0) == 0 && TF2R_GetItemAttributes(0, indexes, values, 0) == -1,
		"it lists no runtime attributes and is no item");

	int player = 0;
	int weapon = -1;
	for (int i = 1; i <= MaxClients && weapon == -1; i++)
	{
		if (!IsClientInGame(i) || !IsPlayerAlive(i))
		{
			continue;
		}

		int active = GetEntPropEnt(i, Prop_Send, "m_hActiveWeapon");
		if (active != -1)
		{
			player = i;
			weapon = active;
		}
	}

	if (weapon == -1)
	{
		PrintToServer("Attribute native tests: %d right, %d WRONG so far. The rest needs a living player who holds a weapon; add a bot and run this again.",
			g_Right, g_Wrong);
		return Plugin_Handled;
	}

	char classname[64];
	GetEntityClassname(weapon, classname, sizeof(classname));
	PrintToServer("Testing on %N and their %s.", player, classname);
	Check(TF2R_HasAttributes(player) && TF2R_HasAttributes(weapon), "the player and the weapon have attributes");

	// A weapon: set, read back, remove
	float own = 1.0;
	bool hadOwn = TF2R_GetItemAttribute(weapon, damage, own) && own != 0.0;
	bool itemHadOne = TF2R_GetItemAttribute(weapon, damage, value);
	float weaponBefore = TF2R_HookValueFloat(weapon, "mult_dmg", 1.0);
	float playerBefore = TF2R_HookValueFloat(player, "mult_dmg", 1.0);
	float factor = hadOwn ? 3.0 / own : 3.0;
	int runtimeBefore = TF2R_GetAttributes(weapon, indexes, values, 0);
	int itemBefore = TF2R_GetItemAttributes(weapon, indexes, values, 0);

	if (TF2R_GetAttribute(weapon, damage, value))
	{
		PrintToServer("The weapon has a runtime \"damage bonus\" already; the checks on it are left out.");
	}
	else
	{
		TF2R_SetAttribute(weapon, damage, 3.0);
		Check(Near(TF2R_HookValueFloat(weapon, "mult_dmg", 1.0), weaponBefore * factor), "a runtime \"damage bonus\" of 3 on the weapon shows in its mult_dmg hook at once");
		Check(Near(TF2R_HookValueFloat(player, "mult_dmg", 1.0), playerBefore * factor), "and in its owner's");
		Check(TF2R_HookValueInt(weapon, "mult_dmg", 100) == RoundToNearest(TF2R_HookValueFloat(weapon, "mult_dmg", 100.0)), "the hook as a whole number is the float one rounded");

		value = -1.0;
		Check(TF2R_GetAttribute(weapon, damage, value) && value == 3.0, "the runtime attribute reads back as 3");
		value = -1.0;
		Check(TF2R_GetAttributeByName(weapon, "damage bonus", value) && value == 3.0, "by its name too");
		value = -1.0;
		Check(TF2R_GetItemAttribute(weapon, damage, value) && value == 3.0, "and is the one the item counts");

		int runtime = TF2R_GetAttributes(weapon, indexes, values, sizeof(indexes));
		bool listed = false;
		for (int i = 0; i < runtime && i < sizeof(indexes); i++)
		{
			if (indexes[i] == damage && values[i] == 3.0)
			{
				listed = true;
			}
		}
		Check(runtime == runtimeBefore + 1 && listed, "the weapon lists one runtime attribute more, this one");

		int item = TF2R_GetItemAttributes(weapon, indexes, values, sizeof(indexes));
		int timesListed = 0;
		bool listedValue = false;
		for (int i = 0; i < item && i < sizeof(indexes); i++)
		{
			if (indexes[i] == damage)
			{
				timesListed++;
				listedValue = (values[i] == 3.0);
			}
		}
		Check(itemBefore >= 0 && item == itemBefore + (itemHadOne ? 0 : 1) && timesListed == 1 && listedValue,
			"the item's own attributes list it once, in the place of the item's");

		TF2R_SetAttributeByName(weapon, "damage bonus", 2.0);
		Check(TF2R_GetAttributes(weapon, indexes, values, 0) == runtime && Near(TF2R_HookValueFloat(weapon, "mult_dmg", 1.0), weaponBefore * factor * 2.0 / 3.0),
			"setting it again, by name, changes the value and adds nothing");

		Check(TF2R_RemoveAttribute(weapon, damage), "it can be removed");
		Check(Near(TF2R_HookValueFloat(weapon, "mult_dmg", 1.0), weaponBefore) && Near(TF2R_HookValueFloat(player, "mult_dmg", 1.0), playerBefore),
			"both hooks are back where they were");
		Check(!TF2R_GetAttribute(weapon, damage, value) && !TF2R_RemoveAttributeByName(weapon, "damage bonus") && TF2R_GetAttributes(weapon, indexes, values, 0) == runtimeBefore,
			"and the runtime attribute is gone");
	}

	// An attribute the game stores as a whole number
	int counter = TF2R_GetAttributeDefIndex("kill eater");
	int whole = -1;
	if (counter >= 0 && TF2R_IsAttributeInteger(counter) && TF2R_IsAttributeNumber(counter) && !TF2R_GetAttributeInt(weapon, counter, whole))
	{
		TF2R_SetAttributeInt(weapon, counter, 12345);
		Check(TF2R_GetAttributeInt(weapon, counter, whole) && whole == 12345 && TF2R_GetAttribute(weapon, counter, value) && view_as<int>(value) == 12345,
			"an attribute stored as a whole number keeps the number it is given");
		Check(TF2R_RemoveAttribute(weapon, counter) && !TF2R_GetAttributeInt(weapon, counter, whole), "and can be removed");
	}
	else
	{
		PrintToServer("\"kill eater\" isn't an attribute stored as a whole number here, or the weapon has one; the checks on one are left out.");
	}

	// A player: a runtime attribute, and one that is timed
	if (TF2R_GetAttribute(player, speed, value) || speedClass[0] == '\0')
	{
		PrintToServer("The player has a runtime \"move speed bonus\" already, or it has no class; the checks on it are left out.");
	}
	else
	{
		float speedBefore = TF2R_HookValueFloat(player, speedClass, 1.0);
		float maxSpeedBefore = GetEntPropFloat(player, Prop_Send, "m_flMaxspeed");

		TF2R_SetAttribute(player, speed, 1.25);
		Check(Near(TF2R_HookValueFloat(player, speedClass, 1.0), speedBefore * 1.25), "a runtime \"move speed bonus\" on the player shows in the hook");
		Check(maxSpeedBefore <= 1.0 || GetEntPropFloat(player, Prop_Send, "m_flMaxspeed") > maxSpeedBefore, "their speed is worked out again without being asked");
		Check(!TF2R_GetItemAttribute(player, speed, value) && TF2R_GetItemAttributes(player, indexes, values, 0) == -1, "a player is no item");
		Check(TF2R_RemoveAttribute(player, speed) && Near(TF2R_HookValueFloat(player, speedClass, 1.0), speedBefore)
			&& Near(GetEntPropFloat(player, Prop_Send, "m_flMaxspeed"), maxSpeedBefore), "removed, hook and speed are back");

		TF2R_AddCustomPlayerAttribute(player, "move speed bonus", 1.25, 60.0);
		Check(Near(TF2R_HookValueFloat(player, speedClass, 1.0), speedBefore * 1.25), "a timed attribute on the player counts the same way");
		TF2R_RemoveCustomPlayerAttribute(player, "move speed bonus");
		Check(Near(TF2R_HookValueFloat(player, speedClass, 1.0), speedBefore), "and is gone when it is taken away");
	}

	// Item definitions, with no entity
	int itemDef = GetEntProp(weapon, Prop_Send, "m_iItemDefinitionIndex");
	TFClassType classType = view_as<TFClassType>(GetEntProp(player, Prop_Send, "m_iClass"));
	int ofDefinitionIndexes[64];
	float ofDefinitionValues[64];
	int ofDefinition = TF2R_GetItemDefinitionAttributes(itemDef, classType, ofDefinitionIndexes, ofDefinitionValues, sizeof(ofDefinitionIndexes));
	Check(ofDefinition >= 0 && TF2R_GetItemDefinitionAttributes(-1, classType, indexes, values, 0) == -1
		&& TF2R_GetItemDefinitionAttributes(1000000, classType, indexes, values, 0) == -1,
		"the weapon's item definition is found, an index that isn't one gives -1");

	// A bot's weapon has nothing from a backpack item, so it has what its item definition has
	if (IsFakeClient(player) && TF2R_GetAttributes(weapon, indexes, values, 0) == 0 && ofDefinition <= sizeof(ofDefinitionIndexes))
	{
		int ofItem = TF2R_GetItemAttributes(weapon, indexes, values, sizeof(indexes));
		bool same = (ofItem == ofDefinition);
		for (int i = 0; same && i < ofItem; i++)
		{
			same = (indexes[i] == ofDefinitionIndexes[i] && view_as<int>(values[i]) == view_as<int>(ofDefinitionValues[i]));
		}
		Check(same, "with nothing set on it, a bot's weapon has what its item definition has for the bot's class");
	}
	else
	{
		PrintToServer("The player isn't a bot, or the weapon has runtime attributes; the comparison with its item definition is left out.");
	}

	// Text
	Check(TF2R_HookValueString(weapon, "tf2r_test_no_such_class", "base text", text, sizeof(text)) && StrEqual(text, "base text"),
		"a text hook nothing answers gives the base text");
	char small[5];
	Check(TF2R_HookValueString(weapon, "tf2r_test_no_such_class", "base text", small, sizeof(small)) && StrEqual(small, "base"),
		"cut to the size it is given");
	Check(!TF2R_HookValueString(0, "tf2r_test_no_such_class", "base text", text, sizeof(text)) && StrEqual(text, "base text"),
		"and says no for the world");
	strcopy(text, sizeof(text), "one array");
	Check(TF2R_HookValueString(weapon, "tf2r_test_no_such_class", text, text, sizeof(text)) && StrEqual(text, "one array"),
		"one array can be the base text and the result");

	if (g_Wrong == 0)
	{
		PrintToServer("Attribute native tests: all %d right.", g_Right);
	}
	else
	{
		PrintToServer("Attribute native tests: %d of %d WRONG.", g_Wrong, g_Right + g_Wrong);
	}

	return Plugin_Handled;
}

public Action Command_Errors(int client, int args)
{
	g_ErrorPlayer = 0;
	g_ErrorWeapon = -1;
	for (int i = 1; i <= MaxClients && g_ErrorWeapon == -1; i++)
	{
		if (!IsClientInGame(i) || !IsPlayerAlive(i))
		{
			continue;
		}

		int active = GetEntPropEnt(i, Prop_Send, "m_hActiveWeapon");
		if (active != -1)
		{
			g_ErrorPlayer = i;
			g_ErrorWeapon = EntIndexToEntRef(active);
		}
	}

	if (g_ErrorWeapon == -1)
	{
		PrintToServer("Attribute native error tests need a living player who holds a weapon; add a bot and run this again.");
		return Plugin_Handled;
	}

	g_ErrorCall = 0;
	RequestFrame(Frame_NextError);
	return Plugin_Handled;
}

void Frame_NextError(any data)
{
	int call = g_ErrorCall++;
	if (call >= ERROR_CALLS)
	{
		PrintToServer("Attribute native error tests: %d calls made, each has to be in the error log.", ERROR_CALLS);
		return;
	}

	// The call ends this function, so the next frame is asked for first
	RequestFrame(Frame_NextError);

	int damage = TF2R_GetAttributeDefIndex("damage bonus");
	int text = TF2R_GetAttributeDefIndex("custom name attr");
	char buffer[8];
	int indexes[1];
	float values[1];

	switch (call)
	{
		case 0: TF2R_SetAttribute(-1, damage, 1.0);
		case 1: TF2R_SetAttribute(0, damage, 1.0);
		case 2: TF2R_SetAttribute(g_ErrorWeapon, 1000000, 1.0);
		case 3: TF2R_SetAttribute(g_ErrorWeapon, text, 1.0);
		case 4: TF2R_SetAttributeByName(g_ErrorWeapon, "no such attribute", 1.0);
		case 5: TF2R_AddCustomPlayerAttribute(0, "move speed bonus", 1.25, 1.0);
		case 6: TF2R_AddCustomPlayerAttribute(g_ErrorPlayer, "custom name attr", 1.0, 1.0);
		case 7: TF2R_GetAttributeName(damage, buffer, 0);
		case 8: TF2R_GetAttributes(g_ErrorPlayer, indexes, values, -1);
	}

	PrintToServer("WRONG  call %d was not refused", call);
}
