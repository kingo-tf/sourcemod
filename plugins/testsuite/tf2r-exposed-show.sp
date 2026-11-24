#include <sourcemod>
#include <sdktools>
#include <sdkhooks>
#include <tf2>
#include <tf2_stocks>

#pragma semicolon 1
#pragma newdecls required

/**
 * Something to watch: puts a bot in front of a player and does to it, one thing every few
 * seconds, what SourceMod can only do when the game's functions and offsets are right. On
 * TF2 Rebalanced those come from what the game exposes by name (tf2r_exposed.inc), so this
 * is that working, seen from inside the game.
 *
 * "test_tf2r_show" in the server console starts it, or "!show" in chat. It waits for a
 * living player and needs a bot, best a Spy ("bot -class spy").
 */

public Plugin myinfo =
{
	name = "TF2R exposed functions show",
	author = "TF2 Rebalanced",
	description = "Does to a bot what needs the game's functions and offsets",
	version = "1.0.0.0",
	url = "https://github.com/kingo-tf/sourcemod"
};

#define ROUNDS			2
#define STEP_SECONDS	3.0

bool g_Running;
int g_Round;
int g_Step;
int g_Waited;
int g_DamageSeen;

public void OnPluginStart()
{
	RegServerCmd("test_tf2r_show", Command_ServerShow, "Starts the show on a bot, for the first living player.");
	RegConsoleCmd("sm_show", Command_Show, "Starts the show on a bot.");

	for (int client = 1; client <= MaxClients; client++)
	{
		if (IsClientInGame(client))
		{
			OnClientPutInServer(client);
		}
	}
}

public void OnClientPutInServer(int client)
{
	SDKHook(client, SDKHook_OnTakeDamage, OnTakeDamage);
}

public void OnMapEnd()
{
	g_Running = false;
}

// SDKHooks: a virtual hook on the player, by the OnTakeDamage offset
public Action OnTakeDamage(int victim, int &attacker, int &inflictor, float &damage, int &damagetype)
{
	if (g_Running && IsFakeClient(victim))
	{
		g_DamageSeen++;
	}

	return Plugin_Continue;
}

public Action Command_ServerShow(int args)
{
	StartShow();
	return Plugin_Handled;
}

public Action Command_Show(int client, int args)
{
	StartShow();
	return Plugin_Handled;
}

void StartShow()
{
	if (g_Running)
	{
		PrintToServer("SHOW already running");
		return;
	}

	g_Running = true;
	g_Round = 1;
	g_Step = 0;
	g_Waited = 0;
	g_DamageSeen = 0;
	PrintToServer("SHOW started");
	CreateTimer(STEP_SECONDS, Timer_Step, _, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
}

int FindHuman()
{
	for (int client = 1; client <= MaxClients; client++)
	{
		if (IsClientInGame(client) && !IsFakeClient(client) && IsPlayerAlive(client))
		{
			return client;
		}
	}

	return 0;
}

int FindBot()
{
	for (int client = 1; client <= MaxClients; client++)
	{
		if (IsClientInGame(client) && IsFakeClient(client) && !IsClientSourceTV(client) && !IsClientReplay(client))
		{
			return client;
		}
	}

	return 0;
}

void Announce(const char[] format, any ...)
{
	char text[192];
	VFormat(text, sizeof(text), format, 2);

	PrintToChatAll("\x04[show %d/%d]\x01 %s", g_Round, ROUNDS, text);
	PrintCenterTextAll("%s", text);
	PrintToServer("SHOW round %d step %d: %s", g_Round, g_Step, text);
}

// 200 units ahead of where the player looks, facing the player
void PlaceBot(int bot, int human)
{
	float origin[3], angles[3], ahead[3];
	GetClientAbsOrigin(human, origin);
	GetClientEyeAngles(human, angles);
	angles[0] = 0.0;
	angles[2] = 0.0;
	GetAngleVectors(angles, ahead, NULL_VECTOR, NULL_VECTOR);

	origin[0] += ahead[0] * 200.0;
	origin[1] += ahead[1] * 200.0;
	origin[2] += 12.0;
	angles[1] += 180.0;

	float still[3];
	TeleportEntity(bot, origin, angles, still);
}

public Action Timer_Step(Handle timer)
{
	if (!g_Running)
	{
		return Plugin_Stop;
	}

	int human = FindHuman();
	int bot = FindBot();
	if (!human || !bot)
	{
		if ((g_Waited++ % 5) == 0)
		{
			PrintToServer("SHOW waiting for %s", human ? "a bot" : "a living player");
			PrintToChatAll("\x04[show]\x01 Waiting for %s.", human ? "a bot" : "you to pick a class and spawn");
		}
		return Plugin_Continue;
	}

	// The bot on the other team, so that what the player does to it counts, and alive
	if (g_Step == 0 && TF2_GetClientTeam(bot) == TF2_GetClientTeam(human))
	{
		ChangeClientTeam(bot, TF2_GetClientTeam(human) == TFTeam_Red ? view_as<int>(TFTeam_Blue) : view_as<int>(TFTeam_Red));
	}
	if (!IsPlayerAlive(bot) && g_Step != 11)
	{
		TF2_RespawnPlayer(bot);
	}

	switch (g_Step)
	{
		case 0:
		{
			PlaceBot(bot, human);
			Announce("The bot is in front of you (TeleportEntity). Keep looking at it.");
		}
		case 1:
		{
			TF2_AddCondition(bot, TFCond_Ubercharged, 3.0);
			Announce("Ubercharged for 3 seconds (TF2_AddCondition)");
		}
		case 2:
		{
			TF2_StunPlayer(bot, 3.0, 0.0, TF_STUNFLAGS_GHOSTSCARE, human);
			Announce("Scared stiff for 3 seconds (TF2_StunPlayer)");
		}
		case 3:
		{
			TF2_MakeBleed(bot, human, 4.0);
			Announce("Bleeding (TF2_MakeBleed), health %d", GetClientHealth(bot));
		}
		case 4:
		{
			TF2_IgnitePlayer(bot, human, 5.0);
			Announce("On fire (TF2_IgnitePlayer)");
		}
		case 5:
		{
			int before = GetClientHealth(bot);
			TF2_RemoveCondition(bot, TFCond_OnFire);
			TF2_RemoveCondition(bot, TFCond_Bleeding);
			TF2_RegeneratePlayer(bot);
			Announce("Put out and resupplied (TF2_RemoveCondition, TF2_RegeneratePlayer): health %d -> %d", before, GetClientHealth(bot));
		}
		case 6:
		{
			if (TF2_GetPlayerClass(bot) == TFClass_Spy)
			{
				TF2_DisguisePlayer(bot, TF2_GetClientTeam(human), TFClass_Heavy);
				Announce("Disguised as a Heavy of your team (TF2_DisguisePlayer)");
			}
			else
			{
				Announce("Not a Spy: no disguise this time");
			}
		}
		case 7:
		{
			TF2_RemovePlayerDisguise(bot);
			Announce("Disguise off (TF2_RemovePlayerDisguise)");
		}
		case 8:
		{
			SlapPlayer(bot, 5, true);
			Announce("Slapped (SlapPlayer)");
		}
		case 9:
		{
			IgniteEntity(bot, 3.0);
			Announce("On fire again, the other way (IgniteEntity)");
		}
		case 10:
		{
			ForcePlayerSuicide(bot);
			Announce("Dead (ForcePlayerSuicide)");
		}
		case 11:
		{
			TF2_RespawnPlayer(bot);
			PlaceBot(bot, human);
			Announce("Back (TF2_RespawnPlayer). SDKHooks saw the bot take damage %d times so far.", g_DamageSeen);
		}
		default:
		{
			if (g_Round < ROUNDS)
			{
				g_Round++;
				g_Step = 0;
				return Plugin_Continue;
			}

			Announce("That was it. \"!show\" in chat runs it again.");
			PrintToServer("SHOW FINISHED, damage seen %d", g_DamageSeen);
			g_Running = false;
			return Plugin_Stop;
		}
	}

	g_Step++;
	return Plugin_Continue;
}
