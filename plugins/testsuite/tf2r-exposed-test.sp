#include <sourcemod>
#include <tf2r_exposed>

#pragma semicolon 1
#pragma newdecls required

public Plugin myinfo =
{
	name = "TF2R exposed functions test",
	author = "TF2 Rebalanced",
	description = "Lists what the game exposes by name",
	version = "1.0.0.0",
	url = "http://www.sourcemod.net/"
};

public void OnPluginStart()
{
	RegServerCmd("test_tf2r_exposed", Command_Test, "Lists what the game exposes by name. Optional argument: one name to look up.");
}

static void PrintExposed(const char[] name, TF2RExposedKind kind)
{
	switch (kind)
	{
		case TF2RExposed_Function:
		{
			PrintToServer("  function  %s  %s", TF2R_GetFunctionAddress(name) != Address_Null ? "found  " : "MISSING", name);
		}
		case TF2RExposed_Virtual:
		{
			PrintToServer("  virtual   %7d  %s", TF2R_GetVTableOffset(name), name);
		}
		case TF2RExposed_Member:
		{
			PrintToServer("  member    %7d  %s", TF2R_GetMemberOffset(name), name);
		}
		default:
		{
			PrintToServer("  global    %s  %s", TF2R_GetGlobalAddress(name) != Address_Null ? "found  " : "MISSING", name);
		}
	}
}

public Action Command_Test(int args)
{
	if (!TF2R_ExposedAvailable())
	{
		PrintToServer("This build of the game exposes nothing by name.");
		return Plugin_Handled;
	}

	if (args >= 1)
	{
		// The whole of it: the console takes "CTFPlayer::Regenerate" for three arguments
		char wanted[256];
		GetCmdArgString(wanted, sizeof(wanted));
		StripQuotes(wanted);
		TrimString(wanted);

		PrintToServer("\"%s\": function %s, vtable offset %d, member offset %d, global %s", wanted,
			TF2R_GetFunctionAddress(wanted) != Address_Null ? "found" : "not found",
			TF2R_GetVTableOffset(wanted),
			TF2R_GetMemberOffset(wanted),
			TF2R_GetGlobalAddress(wanted) != Address_Null ? "found" : "not found");
		return Plugin_Handled;
	}

	int count = TF2R_GetExposedCount();
	for (int i = 0; i < count; i++)
	{
		char name[256];
		TF2RExposedKind kind;
		if (TF2R_GetExposed(i, name, sizeof(name), kind))
		{
			PrintExposed(name, kind);
		}
	}

	// A name that isn't one, and an index that isn't one
	TF2RExposedKind unused;
	char none[4];
	bool wrong = TF2R_GetFunctionAddress("no such function") != Address_Null
		|| TF2R_GetVTableOffset("no such function") != -1
		|| TF2R_GetMemberOffset("no such function") != -1
		|| TF2R_GetGlobalAddress("no such function") != Address_Null
		|| TF2R_GetExposed(count, none, sizeof(none), unused)
		|| TF2R_GetExposed(-1, none, sizeof(none), unused);

	PrintToServer("%d exposed. Unknown names and indexes: %s", count, wrong ? "WRONG" : "ok");
	return Plugin_Handled;
}
