#include <sourcemod>
#include <tf2r_exposed>

#pragma semicolon 1
#pragma newdecls required

/**
 * Checks that SourceMod's gamedata for TF2 Rebalanced really comes from what the game exposes.
 *
 * It reads the game's own gamedata files as they are on the server, and for every entry that
 * refers to an exposed name ("exposed" "CBasePlayer::ForceRespawn") compares what SourceMod's
 * gamedata gives for the key with what the game gives for the name. An entry whose name the
 * game doesn't know is one that fell back to what a shared file has for the key, or has no
 * value at all: after an update of the game, that is the first thing to look for.
 *
 * "test_tf2r_gamedata" in the server console runs it. Every line it prints starts with CHECK;
 * the last one has the totals.
 */

public Plugin myinfo =
{
	name = "TF2R exposed gamedata test",
	author = "TF2 Rebalanced",
	description = "Checks SourceMod's gamedata against what the game exposes",
	version = "1.0.0.0",
	url = "https://github.com/kingo-tf/sourcemod"
};

// The gamedata to load, and the file of it that is this game's
static const char g_Files[][][] =
{
	{ "core.games",		"gamedata/core.games/custom/game.tf2r.txt" },
	{ "sdktools.games",	"gamedata/sdktools.games/custom/game.tf2r.txt" },
	{ "sdkhooks.games",	"gamedata/sdkhooks.games/custom/game.tf2r.txt" },
	{ "sm-tf2r.games",	"gamedata/sm-tf2r.games.txt" }
};

// While a file is read: how deep in it, and the names of the sections on the way.
// "Games" { "tf_ktbm" { "Offsets" { "ForceRespawn" { "exposed" "..." } } } }
int g_Depth;
char g_Section[64];
char g_Key[128];

// What was found: section, key and exposed name, three strings per entry
ArrayList g_Found;

public void OnPluginStart()
{
	RegServerCmd("test_tf2r_gamedata", Command_Check, "Checks every gamedata entry that refers to an exposed name.");
}

public SMCResult Parse_EnterSection(SMCParser smc, const char[] name, bool opt_quotes)
{
	g_Depth++;
	if (g_Depth == 3)
	{
		strcopy(g_Section, sizeof(g_Section), name);
	}
	else if (g_Depth == 4)
	{
		strcopy(g_Key, sizeof(g_Key), name);
	}

	return SMCParse_Continue;
}

public SMCResult Parse_LeaveSection(SMCParser smc)
{
	g_Depth--;
	return SMCParse_Continue;
}

public SMCResult Parse_KeyValue(SMCParser smc, const char[] key, const char[] value, bool key_quotes, bool value_quotes)
{
	if (g_Depth == 4 && StrEqual(key, "exposed") && (StrEqual(g_Section, "Offsets") || StrEqual(g_Section, "Signatures")))
	{
		g_Found.PushString(g_Section);
		g_Found.PushString(g_Key);
		g_Found.PushString(value);
	}

	return SMCParse_Continue;
}

public Action Command_Check(int args)
{
	if (!TF2R_ExposedAvailable())
	{
		PrintToServer("CHECK FAILED: the game exposes nothing by name");
		return Plugin_Handled;
	}

	int entries, right, wrong;
	StringMap referenced = new StringMap();

	for (int file = 0; file < sizeof(g_Files); file++)
	{
		char path[PLATFORM_MAX_PATH];
		BuildPath(Path_SM, path, sizeof(path), "%s", g_Files[file][1]);

		g_Found = new ArrayList(ByteCountToCells(256));
		g_Depth = 0;

		SMCParser parser = new SMCParser();
		parser.OnEnterSection = Parse_EnterSection;
		parser.OnLeaveSection = Parse_LeaveSection;
		parser.OnKeyValue = Parse_KeyValue;
		int line, col;
		SMCError error = parser.ParseFile(path, line, col);
		delete parser;

		GameData conf = LoadGameConfigFile(g_Files[file][0]);
		if (error != SMCError_Okay || conf == null)
		{
			wrong++;
			PrintToServer("CHECK WRONG  %s could not be %s (%s, line %d)", g_Files[file][0], conf == null ? "loaded" : "read", g_Files[file][1], line);
			delete g_Found;
			delete conf;
			continue;
		}

		for (int i = 0; i + 2 < g_Found.Length; i += 3)
		{
			char section[64], key[128], name[256];
			g_Found.GetString(i, section, sizeof(section));
			g_Found.GetString(i + 1, key, sizeof(key));
			g_Found.GetString(i + 2, name, sizeof(name));

			entries++;
			referenced.SetValue(name, 1);

			if (StrEqual(section, "Offsets"))
			{
				// An offset is a virtual function's place in its vtable, or a member variable's in its class
				int want = TF2R_GetVTableOffset(name);
				bool member = false;
				if (want < 0)
				{
					want = TF2R_GetMemberOffset(name);
					member = true;
				}
				int got = conf.GetOffset(key);

				if (want < 0)
				{
					wrong++;
					PrintToServer("CHECK WRONG  %-15s offset    %-34s NOT EXPOSED BY THE GAME: %s (gamedata gives %d)", g_Files[file][0], key, name, got);
				}
				else if (got != want)
				{
					wrong++;
					PrintToServer("CHECK WRONG  %-15s offset    %-34s gamedata %d, game %d: %s", g_Files[file][0], key, got, want, name);
				}
				else
				{
					right++;
					PrintToServer("CHECK ok     %-15s %-9s %-34s %6d  %s", g_Files[file][0], member ? "member" : "virtual", key, got, name);
				}
			}
			else
			{
				// A signature is a function, or a global variable
				Address want = TF2R_GetFunctionAddress(name);
				bool global = false;
				if (want == Address_Null)
				{
					want = TF2R_GetGlobalAddress(name);
					global = true;
				}
				Address got = conf.GetMemSig(key);

				if (want == Address_Null)
				{
					wrong++;
					PrintToServer("CHECK WRONG  %-15s signature %-34s NOT EXPOSED BY THE GAME: %s (gamedata %s)", g_Files[file][0], key, name, got != Address_Null ? "has an address all the same" : "has none");
				}
				else if (got != want)
				{
					wrong++;
					PrintToServer("CHECK WRONG  %-15s signature %-34s gamedata's address is not the game's: %s", g_Files[file][0], key, name);
				}
				else
				{
					right++;
					PrintToServer("CHECK ok     %-15s %-9s %-34s   same  %s", g_Files[file][0], global ? "global" : "function", key, name);
				}
			}
		}

		delete g_Found;
		delete conf;
	}

	// What the game exposes that no gamedata entry refers to: there for plugins only
	int count = TF2R_GetExposedCount();
	int unreferenced;
	for (int i = 0; i < count; i++)
	{
		char name[256];
		TF2RExposedKind kind;
		int unused;
		if (TF2R_GetExposed(i, name, sizeof(name), kind) && !referenced.GetValue(name, unused))
		{
			unreferenced++;
			PrintToServer("CHECK note   exposed by the game, in no gamedata entry: %s", name);
		}
	}
	delete referenced;

	PrintToServer("CHECK DONE: %d entries, %d right, %d wrong; the game exposes %d names, %d of them in no gamedata entry", entries, right, wrong, count, unreferenced);
	return Plugin_Handled;
}
