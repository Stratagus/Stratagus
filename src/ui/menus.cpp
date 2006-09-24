//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
/**@name menus.cpp - The menu function code. */
//
//      (c) Copyright 1999-2006 by Andreas Arens, Jimmy Salmon, Nehal Mistry
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//
//      $Id$

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

#include "stratagus.h"

#include "SDL.h"

#include "iocompat.h"

#include "unittype.h"
#include "video.h"
#include "player.h"
#include "font.h"
#include "tileset.h"
#include "map.h"
#include "minimap.h"
#include "interface.h"
#include "menus.h"
#include "cursor.h"
#include "iolib.h"
#include "network.h"
#include "netconnect.h"
#include "settings.h"
#include "ui.h"
#include "campaign.h"
#include "sound_server.h"
#include "sound.h"
#include "script.h"
#include "editor.h"
#include "commands.h"
#include "actions.h"
#include "net_lowlevel.h"
#include "master.h"
#include "construct.h"


/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

#if 0
	/// Editor cancel button pressed
static int EditorCancelled;
#endif

/**
** Other client and server selection state for Multiplayer clients
*/
ServerSetup ServerSetupState;
ServerSetup LocalSetupState;

static char ScenSelectPath[1024];        /// Scenario selector path
#if 0
static char ScenSelectDisplayPath[1024]; /// Displayed selector path
#endif
static char ScenSelectFileName[128];     /// Scenario selector name

char MenuMapFullPath[1024];              /// Selected map path+name

#if 0
/// FIXME: -> ccl...
static char *mgptsoptions[] = {
	"Available",
	"Computer",
	"Closed",
};
#endif

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

#if 0
/**
** Load game ok button callback
*/
static void LoadGameOk(void)
{
	if (!ScenSelectFileName[0]) {
		return ;
	}
	sprintf(TempPathBuf, "%s/%s", SaveDir, ScenSelectFileName);
	CommandLogDisabled = 1;
	SaveGameLoading = 1;
	CleanModules();
	LoadCcl();
	LoadGame(TempPathBuf);
	Callbacks = &GameCallbacks;
	SetMessage(_("Loaded game: %s"), TempPathBuf);
	GuiGameStarted = 1;
	GameMenuReturn();
	SelectedFileExist = 0;
	ScenSelectFileName[0] = '\0';
	ScenSelectPathName[0] = '\0';
}
#endif

#if 0
/**
** Save replay Ok button.
*/
static void SaveReplayOk(void)
{
	FILE *fd;
	Menu *menu;
	char *buf;
	struct stat s;
	char *ptr;

	menu = FindMenu("menu-save-replay");

	if (strchr(menu->Items[1].D.Input.buffer, '/')) {
		ErrorMenu("Name cannot contain '/'");
		return;
	}
	if (strchr(menu->Items[1].D.Input.buffer, '\\')) {
		ErrorMenu("Name cannot contain '\\'");
		return;
	}

#ifdef WIN32
	sprintf(TempPathBuf, "%s/logs/",GameName);
#else
	sprintf(TempPathBuf, "%s/%s/%s", getenv("HOME"), STRATAGUS_HOME_PATH,GameName);
	strcat(TempPathBuf, "/logs/");
#endif
	ptr = TempPathBuf + strlen(TempPathBuf);
	sprintf(ptr, "log_of_stratagus_%d.log", ThisPlayer->Index);

	stat(TempPathBuf, &s);
	buf = new char[s.st_size];
	fd = fopen(TempPathBuf, "rb");
	fread(buf, s.st_size, 1, fd);
	fclose(fd);

	strncpy(ptr, menu->Items[1].D.Input.buffer, menu->Items[1].D.Input.nch);
	ptr[menu->Items[1].D.Input.nch] = '\0';
	if (!strcasestr(menu->Items[1].D.Input.buffer, ".log")) {
		strcat(ptr, ".log");
	}

	fd = fopen(TempPathBuf, "wb");
	if (!fd) {
		ErrorMenu("Cannot write to file");
		delete[] buf;
		return;
	}
	fwrite(buf, s.st_size, 1, fd);
	fclose(fd);

	delete[] buf;
	//CloseMenu();
	SelectedFileExist = 0;
	ScenSelectFileName[0] = '\0';
	ScenSelectPathName[0] = '\0';
}
#endif

#if 0
/**
**  Cancel button of network connect menu pressed.
*/
static void NetConnectingCancel(void)
{
	NetworkExitClientConnect();
	// Trigger TerminateNetConnect() to call us again and end the menu
	NetLocalState = ccs_usercanceled;
	//CloseMenu();
}
#endif

#if 0
/**
**  Call back from menu loop, if network state has changed.
*/
static void TerminateNetConnect(void)
{
	switch (NetLocalState) {
		case ccs_unreachable:
			NetErrorMenu("Cannot reach server.");
			NetConnectingCancel();
			return;
		case ccs_nofreeslots:
			NetErrorMenu("Server is full.");
			NetConnectingCancel();
			return;
		case ccs_serverquits:
			NetErrorMenu("Server gone.");
			NetConnectingCancel();
			return;
		case ccs_incompatibleengine:
			NetErrorMenu("Incompatible engine version.");
			NetConnectingCancel();
			return;
		case ccs_badmap:
			NetErrorMenu("Map not available.");
			NetConnectingCancel();
			return;
		case ccs_incompatiblenetwork:
			NetErrorMenu("Incompatible network version.");
			NetConnectingCancel();
			return;
		case ccs_usercanceled:
			NetConnectingCancel();
			return;
		case ccs_started:
			NetworkGamePrepareGameSettings();
			CustomGameStart();
			return;
		default:
			break;
	}

	DebugPrint("NetLocalState %d\n" _C_ NetLocalState);
	NetConnectRunning = 2;
	GuiGameStarted = 0;
	ProcessMenu("menu-net-multi-client", 1);
	if (GuiGameStarted) {
		GameMenuReturn();
	} else {
		NetConnectingCancel();
	}
}
#endif

#if 0
/**
** Multiplayer game start game button pressed.
*/
static void MultiGameStart(void)
{
	if (MetaServerInUse) {
		SendMetaCommand("StartGame", "");
	}

	GameSettings.Presets[0].Race = SettingsPresetMapDefault;

	NetworkServerStartGame();
	NetworkGamePrepareGameSettings();

	CustomGameStart();
}
#endif

#if 0
/**
** Enter multiplayer game menu.
*/
static void MultiPlayerGameMenu(void)
{
	char NameBuf[32];
	Menu *menu;

	menu = FindMenu("menu-enter-name");
	menu->Items[1].D.Input.buffer = NameBuf;
	strcpy(NameBuf, LocalPlayerName);
	strcat(NameBuf, "~!_");
	menu->Items[1].D.Input.nch = strlen(NameBuf) - 3;
	menu->Items[1].D.Input.maxch = 15;
	menu->Items[2].Flags &= ~MI_FLAGS_DISABLED;

	ProcessMenu("menu-enter-name", 1);

	if (menu->Items[1].D.Input.nch == 0) {
		return;
	}

	NameBuf[menu->Items[1].D.Input.nch] = 0; // Now finally here is the name
	memset(LocalPlayerName, 0, 16);
	strcpy(LocalPlayerName, NameBuf);

	// FIXME: Only save if player name changed
	SavePreferences();

	GuiGameStarted = 0;
	if (!MasterHost) {
		// No Metaserver Configured - Go right to LAN Game.
		ProcessMenu("menu-create-join-menu", 1);
	} else {
		ProcessMenu("menu-multi-net-type-menu", 1);
	}


	DebugPrint("GuiGameStarted: %d\n" _C_ GuiGameStarted);
	if (GuiGameStarted) {
		GameMenuReturn();
	}
}
#endif

#if 0
/**
** Process LAN/P2P game menu
*/
static void MultiPlayerLANGame(void)
{
	ProcessMenu("menu-create-join-menu", 1);
	MetaServerInUse = 0;
	if (GuiGameStarted) {
		GameMenuReturn();
	}
}
#endif

#if 0
/**
** Process Internet game menu
*/
static void MultiPlayerInternetGame(void)
{
	//Connect to Meta Server
	if (MetaInit() == -1 ) {
		MetaServerInUse = 0;
		MetaServerConnectError();
		return;
	}
	MetaServerInUse = 1;
	ProcessMenu("menu-internet-create-join-menu", 1);
	if (GuiGameStarted) {
		GameMenuReturn();
	}
}
#endif

#if 0
/**
**  Scenario select map type action callback
*/
static void ScenSelectTPMSAction(Menuitem *mi, int i)
{
	mi = mi->Menu->Items + 1;
	ScenSelectLBInit(mi);
}
#endif

#if 0
/**
** Custom game start game button pressed.
*/
static void CustomGameStart(void)
{
	int i;
	char *p;

	if (ScenSelectPath[0]) {
		strcat(ScenSelectPath, "/");
		strcat(ScenSelectPath, ScenSelectFileName); // Final map name with path
		p = ScenSelectPath + strlen(StratagusLibPath) + 1;
		strcpy(CurrentMapPath, p);
	} else {
		strcpy(CurrentMapPath, ScenSelectFileName);
		strcat(ScenSelectPath, ScenSelectFileName); // Final map name with path
	}

	for (i = 0; i < MAX_OBJECTIVES; i++) {
		if (GameIntro.Objectives[i]) {
			delete[] GameIntro.Objectives[i];
			GameIntro.Objectives[i] = NULL;
		}
	}
	GameIntro.Objectives[0] = new_strdup(DefaultObjective);

	GuiGameStarted = 1;
	//CloseMenu();
}
#endif

#if 0
/**
** Single player custom game menu entered.
*/
static void GameSetupInit(Menu *menu)
{
	Menu *custom_menu;
	int n;
	int j;
	int t;
	char *s;

	//
	//  No old path, setup the default.
	//
	if (!*CurrentMapPath || *CurrentMapPath == '.' || *CurrentMapPath == '/') {
		strcpy(CurrentMapPath, DefaultMap);
	}

	DebugPrint("Map   path: %s\n" _C_ CurrentMapPath);
	strcpy(ScenSelectPath, StratagusLibPath);
	if (*ScenSelectPath) {
		strcat(ScenSelectPath, "/");
	}
	strcat(ScenSelectPath, CurrentMapPath);
	if ((s = strrchr(ScenSelectPath, '/'))) {
		strcpy(ScenSelectFileName, s + 1);
		*s = '\0';
	}
	strcpy(ScenSelectDisplayPath, CurrentMapPath);
	if ((s = strrchr(ScenSelectDisplayPath, '/'))) {
		*s = '\0';
	} else {
		*ScenSelectDisplayPath = '\0';
	}
	DebugPrint("Start path: %s\n" _C_ ScenSelectPath);

	GetInfoFromSelectPath();

	custom_menu = FindMenu("menu-custom-game");
	for (n = j = 0; j < PlayerMax; ++j) {
		t = Map.Info.PlayerType[j];
		if (t == PlayerPerson || t == PlayerComputer) {
			++n;
		}
	}
	custom_menu->Items[12].D.Pulldown.noptions = n;
	if (custom_menu->Items[12].D.Pulldown.curopt >= n) {
		custom_menu->Items[12].D.Pulldown.curopt = 0;
	}
}
#endif

#if 0
/**
**  Menu setup race pulldown action.
**
**  @note 0 is default-race.
*/
static void GameRCSAction(Menuitem *mi, int i)
{
	int n;
	int x;

	if (mi->D.Pulldown.curopt == i) {
		for (n = 0, x = 0; n < PlayerRaces.Count; ++n) {
			if (PlayerRaces.Visible[n]) {
				if (x + 1 == i) {
					break;
				}
				++x;
			}
		}
		if (i != 0) {
			GameSettings.Presets[0].Race = x;
		} else {
			GameSettings.Presets[0].Race = SettingsPresetMapDefault;
		}
		ServerSetupState.Race[0] = i;
		NetworkServerResyncClients();
	}
}
#endif

#if 0
/**
** Game resources action callback
*/
static void GameRESAction(Menuitem *mi, int i)
{
	int v[] = { SettingsResourcesMapDefault, SettingsResourcesLow,
				SettingsResourcesMedium, SettingsResourcesHigh };

	if (!mi || mi->D.Pulldown.curopt == i) {
		GameSettings.Resources = v[i];
		ServerSetupState.ResourcesOption = i;
		if (mi) {
			NetworkServerResyncClients();
		}
	}
}

/**
** Game units action callback
*/
static void GameUNSAction(Menuitem *mi, int i)
{
	if (!mi || mi->D.Pulldown.curopt == i) {
		GameSettings.NumUnits = i ? SettingsNumUnits1 : SettingsNumUnitsMapDefault;
		ServerSetupState.UnitsOption = i;
		if (mi) {
			NetworkServerResyncClients();
		}
	}
}

/**
** Game tilesets action callback
*/
static void GameTSSAction(Menuitem *mi, int i)
{
	if (!mi || mi->D.Pulldown.curopt == i) {
		// Subtract 1 for default option.
		GameSettings.Terrain = i - 1;
		ServerSetupState.TilesetSelection = i;
		if (mi) {
			NetworkServerResyncClients();
		}
	}
}

/**
** Called if the pulldown menu of the game type is changed.
*/
static void GameGATAction(Menuitem *mi, int i)
{
	if (!mi || mi->D.Pulldown.curopt == i) {
		GameSettings.GameType = i ? SettingsGameTypeMelee + i - 1 : SettingsGameTypeMapDefault;
		ServerSetupState.GameTypeOption = i;
		if (mi) {
			NetworkServerResyncClients();
		}
	}
}
#endif

#if 0
/**
** Game opponents action callback
*/
static void CustomGameOPSAction(Menuitem *mi, int i)
{
	GameSettings.Opponents = i ? i : SettingsPresetMapDefault;
}
#endif

#if 0
/**
** Menu setup fog-of-war pulldown action.
*/
static void MultiGameFWSAction(Menuitem *mi, int i)
{
	if (!mi || mi->D.Pulldown.curopt == i) {
		DebugPrint("Update fow %d\n" _C_ i);
		switch (i) {
			case 0:
				Map.NoFogOfWar = false;
				FlagRevealMap = 0;
				GameSettings.NoFogOfWar = false;
				GameSettings.RevealMap = 0;
				break;
			case 1:
				Map.NoFogOfWar = true;
				FlagRevealMap = 0;
				GameSettings.NoFogOfWar = true;
				GameSettings.RevealMap = 0;
				break;
			case 2:
				Map.NoFogOfWar = false;
				FlagRevealMap = 1;
				GameSettings.NoFogOfWar = false;
				GameSettings.RevealMap = 1;
				break;
			case 3:
				Map.NoFogOfWar = true;
				FlagRevealMap = 1;
				GameSettings.NoFogOfWar = true;
				GameSettings.RevealMap = 1;
				break;
		}
		ServerSetupState.FogOfWar = i;
		if (mi) {
			NetworkServerResyncClients();
		}
	}
}
#endif

#if 0
/**
** Multiplayer menu player server states.
*/
static void MultiGamePTSAction(Menuitem *mi, int o)
{
	Menu *menu;
	int i;

	menu = FindMenu("menu-multi-setup");
	i = mi - menu->Items - SERVER_PLAYER_STATE;
	// JOHNS: Must this be always true?
	// ARI: NO! think of client menus!
	// Assert(i >= 0 && i <= PlayerMax - 1);

	if (i > 0 && i < PlayerMax-1) {
		if (mi->D.Pulldown.curopt == o) {
			if (mi->D.Pulldown.noptions == 2) { // computer slot
				ServerSetupState.CompOpt[i] = o + 1;
			} else {
				ServerSetupState.CompOpt[i] = o;
			}
			MultiGamePlayerSelectorsUpdate(3); // Recalc buttons on server
			NetworkServerResyncClients();
		}
	}

}
#endif

#if 0
/**
** Player selectors have changed.
** Caution: Called by map change (initial = 1)!
** Caution: Called by network events from clients (initial = 2)!
** Caution: Called by button action on server (initial = 3)!
*/
static void MultiGamePlayerSelectorsUpdate(int initial)
{
	Menu *menu;
	int i;
	int h;
	int c;
	int avail;
	int ready;
	int plyrs;

	menu = FindMenu("menu-multi-setup");

	// FIXME: What this has to do:
	// Use lag gem as KICK button
	//  Notify clients about MAP change: (initial = 1...)

	// Calculate available slots from map info
	for (c = h = i = 0; i < PlayerMax; i++) {
		if (Map.Info.PlayerType[i] == PlayerPerson) {
			h++; // available interactive player slots
		}
		if (Map.Info.PlayerType[i] == PlayerComputer) {
			c++; // available computer player slots
		}
	}

	avail = h;
	plyrs = 0;
	// Setup the player menu
	for (ready = i = 1; i < PlayerMax-1; i++) {
		if (initial == 1) {
			if (i < h) {
				ServerSetupState.CompOpt[i] = 0;
			}
			menu->Items[SERVER_PLAYER_READY - 1 + i].Flags = MI_FLAGS_DISABLED;
			menu->Items[SERVER_PLAYER_LAG - 1 + i].Flags = MI_FLAGS_DISABLED;

			// FIXME: don't forget to throw out additional players
			//   without available slots here!

		}
		if (Hosts[i].PlyNr) {
			menu->Items[SERVER_PLAYER_STATE + i].Flags |= MI_FLAGS_INVISIBLE;
			menu->Items[SERVER_PLAYER_TEXT + i].Flags &= ~MI_FLAGS_INVISIBLE;

			menu->Items[SERVER_PLAYER_READY - 1 + i].Flags = MI_FLAGS_DISABLED;
			++plyrs;
			if (ServerSetupState.Ready[i]) {
				menu->Items[SERVER_PLAYER_READY - 1 + i].D.Checkbox.Checked = 1;
				++ready;
			}

			menu->Items[SERVER_PLAYER_LAG - 1 + i].Flags = MI_FLAGS_DISABLED;
		} else {
			// don't allow network and button events to intercept server player's action on pulldown buttons!
			if (!(menu->Items[SERVER_PLAYER_STATE + i].Flags & MI_FLAGS_CLICKED)) {
				if (initial == 1 ||
					(initial == 2 && menu->Items[SERVER_PLAYER_STATE + i].MiType != MiTypePulldown)) {
					menu->Items[SERVER_PLAYER_STATE + i].Flags = MI_FLAGS_NONE;
					menu->Items[SERVER_PLAYER_TEXT + i].Flags |= MI_FLAGS_INVISIBLE;
					menu->Items[SERVER_PLAYER_STATE + i].D.Pulldown.curopt = ServerSetupState.CompOpt[i];
				}
			}
			if (i < h && ServerSetupState.CompOpt[i] != 0) {
				avail--;
			}

			menu->Items[SERVER_PLAYER_READY - 1 + i].Flags = MI_FLAGS_DISABLED;
			menu->Items[SERVER_PLAYER_LAG - 1 + i].Flags = MI_FLAGS_DISABLED;
		}


		if (i >= h) {
			// Allow to switch off (close) preset ai-computer slots
			// FIXME: evaluate game type...
			if (initial == 1 && i < h + c) {
				menu->Items[SERVER_PLAYER_STATE + i].Flags &= ~MI_FLAGS_DISABLED;
				menu->Items[SERVER_PLAYER_STATE + i].D.Pulldown.noptions = 2;
				menu->Items[SERVER_PLAYER_STATE + i].D.Pulldown.options = mgptsoptions + 1;
				menu->Items[SERVER_PLAYER_STATE + i].D.Pulldown.curopt = 0;
				ServerSetupState.CompOpt[i] = 1;
				menu->Items[SERVER_PLAYER_STATE + i].D.Pulldown.curopt = ServerSetupState.CompOpt[i] - 1;
			}

			menu->Items[SERVER_PLAYER_READY - 1 + i].Flags = MI_FLAGS_DISABLED;
			menu->Items[SERVER_PLAYER_LAG - 1 + i].Flags = MI_FLAGS_DISABLED;
		}

		if (i >= h + c) {
			menu->Items[SERVER_PLAYER_STATE + i].D.Pulldown.defopt = 2;
			menu->Items[SERVER_PLAYER_STATE + i].D.Pulldown.curopt = 2;
			menu->Items[SERVER_PLAYER_STATE + i].Flags = MI_FLAGS_DISABLED;
		}
	}


	// Tell connect state machines how many interactive players we can have
	NetPlayers = avail;
	// Check if all players are ready.
	DebugPrint("READY to START: AVAIL = %d, READY = %d\n" _C_ avail
			_C_ ready);

	// Disable the select scenario after players have joined.
	if (plyrs) {
		// disable Select Scenario button
		menu->Items[2].Flags = MI_FLAGS_DISABLED;
	} else {
		// enable Select Scenario button
		menu->Items[2].Flags = 0;
	}
	if (ready == avail) {
		if (menu->Items[3].Flags == MI_FLAGS_DISABLED) {
			// enable start game button
			menu->Items[3].Flags = 0;
		}
	} else {
		// disable start game button
		menu->Items[3].Flags = MI_FLAGS_DISABLED;
	}

	if (MetaServerInUse) {
		ChangeGameServer();
	}
}
#endif

#if 0
/**
** Update client network menu.
*/
static void MultiClientUpdate(int initial)
{
	Menu *menu;
	int i;
	int h;
	int c;

	menu = FindMenu("menu-net-multi-client");

	//  Calculate available slots from map info
	for (c = h = i = 0; i < PlayerMax; i++) {
		if (Map.Info.PlayerType[i] == PlayerPerson) {
			h++; // available interactive player slots
		}
		if (Map.Info.PlayerType[i] == PlayerComputer) {
			c++; // available computer player slots
		}
	}

	//
	// Setup defaults, reset values.
	//
	if (initial) {
		menu->Items[CLIENT_PLAYER_STATE].Flags |= MI_FLAGS_INVISIBLE;
		menu->Items[CLIENT_PLAYER_TEXT].Flags &= ~MI_FLAGS_INVISIBLE;
		memset(&ServerSetupState, 0, sizeof(ServerSetup));
		memset(&LocalSetupState, 0, sizeof(ServerSetup));
	}
	for (i = 1; i < PlayerMax - 1; i++) {
		//
		// Johns: This works only if initial. Hosts[i].PlyNr is later lost.
		//
		if (Hosts[i].PlyNr || i == NetLocalHostsSlot) {
			menu->Items[CLIENT_PLAYER_STATE + i].Flags |= MI_FLAGS_INVISIBLE;
			menu->Items[CLIENT_PLAYER_TEXT + i].Flags &= ~MI_FLAGS_INVISIBLE;
			if (i == NetLocalHostsSlot) {
				menu->Items[CLIENT_PLAYER_READY - 1 + i].Flags = MI_FLAGS_DISABLED;
			} else {
				menu->Items[CLIENT_PLAYER_READY - 1 + i].Flags = MI_FLAGS_NONE;
			}
		} else {
			menu->Items[CLIENT_PLAYER_STATE + i].Flags &= ~MI_FLAGS_INVISIBLE;
			menu->Items[CLIENT_PLAYER_TEXT + i].Flags |= MI_FLAGS_INVISIBLE;
			menu->Items[CLIENT_PLAYER_STATE + i].Flags |= MI_FLAGS_DISABLED;
			menu->Items[CLIENT_PLAYER_STATE + i].D.Pulldown.curopt =
				ServerSetupState.CompOpt[i];
			menu->Items[CLIENT_PLAYER_READY - 1 + i].Flags = MI_FLAGS_INVISIBLE;
		}

		menu->Items[CLIENT_PLAYER_READY - 1 + i].D.Checkbox.Checked = !!ServerSetupState.Ready[i];

#if 0
		if (i != NetLocalHostsSlot) {
		//if (i >= h) {
			menu->Items[CLIENT_PLAYER_STATE + i].D.Pulldown.curopt =
				ServerSetupState.CompOpt[i];
		}
#endif

		// Unused slots are always disabled.
		if (i >= h + c) {
			menu->Items[CLIENT_PLAYER_READY - 1 + i].Flags =
				MI_FLAGS_INVISIBLE;
			menu->Items[CLIENT_PLAYER_STATE + i].D.Pulldown.defopt =
				menu->Items[CLIENT_PLAYER_STATE + i].D.Pulldown.curopt = 2;
			menu->Items[CLIENT_PLAYER_STATE + i].Flags = MI_FLAGS_DISABLED;
		}
	}
}
#endif

#if 0
/**
**  Multiplayer server menu init callback
*/
static void MultiGameSetupInit(Menu *menu)
{
	int i;
	int h;

	// FIXME: Remove this when .cm is supported
	if (*CurrentMapPath && strstr(CurrentMapPath, ".cm\0")) {
		*CurrentMapPath = '\0';
	}

	GameSetupInit(menu);
	NetworkInitServerConnect();
	menu->Items[SERVER_PLAYER_STATE].Flags |= MI_FLAGS_INVISIBLE;
	MultiGameFWSAction(NULL, menu->Items[27].D.Pulldown.defopt);

	memset(&ServerSetupState, 0, sizeof(ServerSetup));
	// Calculate available slots from map info
	for (h = i = 0; i < PlayerMax; i++) {
		if (Map.Info.PlayerType[i] == PlayerPerson) {
			++h; // available interactive player slots
		}
	}
	for (i = h; i < PlayerMax - 1; ++i) {
		ServerSetupState.CompOpt[i] = 1;
	}
	MultiGamePlayerSelectorsUpdate(1);

	if (MetaServerInUse) {
		ChangeGameServer();
	}

}
#endif

#if 0
/**
**  Cancel button of server multi player menu pressed.
*/
static void MultiGameCancel(void)
{
	NetworkExitServerConnect();

	if (MetaServerInUse) {
		SendMetaCommand("AbandonGame", "");
	}

	FreeMapInfo(&Map.Info);

	NetPlayers = 0; // Make single player menus work again!
	GameCancel();
}
#endif

#if 0
/**
**  Draw the multi player setup menu.
*/
static void NetMultiPlayerDrawFunc(Menuitem *mi)
{
	Menu *menu;
	int i;
	const char *nc;
	const char *rc;

	menu = FindMenu("menu-multi-setup");
	i = mi - menu->Items - SERVER_PLAYER_TEXT;
	if (i >= 0 && i < PlayerMax - 1) { // Ugly test to detect server
		if (i > 0) {
			menu->Items[SERVER_PLAYER_READY - 1 + i].Flags &=
				~MI_FLAGS_DISABLED;
			// Note: re-disabled in MultiGamePlayerSelectorsUpdate()
			// for kicked out clients!!
			if (ServerSetupState.Ready[i]) {
				menu->Items[SERVER_PLAYER_READY - 1 + i].Flags |= MI_FLAGS_DISABLED;
				menu->Items[SERVER_PLAYER_READY - 1 + i].D.Checkbox.Checked = 1;
			} else {
				menu->Items[SERVER_PLAYER_READY - 1 + i].Flags |= MI_FLAGS_DISABLED;
				menu->Items[SERVER_PLAYER_READY - 1 + i].D.Checkbox.Checked = 0;
			}
			if (ServerSetupState.LastFrame[i] + 30 > FrameCounter) {
				menu->Items[SERVER_PLAYER_LAG - 1 + i].Flags |= MI_FLAGS_DISABLED;
				menu->Items[SERVER_PLAYER_LAG - 1 + i].D.Checkbox.Checked = 1;
			} else {
				menu->Items[SERVER_PLAYER_LAG - 1 + i].Flags |= MI_FLAGS_DISABLED;
			}

		}
	} else {
		menu = FindMenu("menu-net-multi-client");
		i = mi - menu->Items - CLIENT_PLAYER_TEXT;
		if (i > 0) {
			menu->Items[CLIENT_PLAYER_READY - 1 + i].Flags &=
				~MI_FLAGS_DISABLED;
			if (i == NetLocalHostsSlot) {
				menu->Items[CLIENT_PLAYER_READY - 1 + i].Flags &= ~MI_FLAGS_DISABLED;
			} else {
				menu->Items[CLIENT_PLAYER_READY - 1 + i].Flags |= MI_FLAGS_DISABLED;
			}
		}
	}

	GetDefaultTextColors(&nc, &rc);
	SetDefaultTextColors(rc, rc);
	VideoDrawText(UI.Offset640X + mi->XOfs, UI.Offset480Y + mi->YOfs, GameFont, Hosts[i].PlyName);

	SetDefaultTextColors(nc, rc);
}
#endif

#if 0
/**
**  Cancel button of multiplayer client menu pressed.
*/
static void MultiClientCancel(void)
{
	NetworkDetachFromServer();
	FreeMapInfo(&Map.Info);
	// GameCancel();
}
#endif

#if 0
/**
**  Multiplayer client menu init callback
*/
static void MultiGameClientInit(Menu *menu)
{
	// GameSetupInit(mi);
	MultiClientUpdate(1);
	if (LocalSetupState.Ready[NetLocalHostsSlot]) {
		menu->Items[2].Flags = MI_FLAGS_DISABLED;
		menu->Items[3].Flags = 0;
		menu->Items[CLIENT_RACE].Flags = MI_FLAGS_DISABLED;
	} else {
		menu->Items[3].Flags = MI_FLAGS_DISABLED;
		menu->Items[2].Flags = 0;
		menu->Items[CLIENT_RACE].Flags = 0;
	}
}
#endif

#if 0
/**
**  Multiplayer client gem action. Toggles ready flag.
*/
static void MultiClientCheckboxAction(Menuitem *mi)
{
	int i;

	i = mi - mi->Menu->Items - CLIENT_PLAYER_READY + 1;
	if (i == NetLocalHostsSlot) {
		LocalSetupState.Ready[i] = !LocalSetupState.Ready[i];
		if (LocalSetupState.Ready[i]) {
			mi->Menu->Items[2].Flags = MI_FLAGS_DISABLED;
			mi->Menu->Items[3].Flags = 0;
			mi->Menu->Items[CLIENT_RACE].Flags = MI_FLAGS_DISABLED;
		} else {
			mi->Menu->Items[3].Flags = MI_FLAGS_DISABLED;
			mi->Menu->Items[2].Flags = 0;
			mi->Menu->Items[CLIENT_RACE].Flags = 0;
		}
		MultiClientUpdate(0);
	}
}
#endif

#if 0
/**
**  Multiplayer client races action callback
*/
static void MultiClientRCSAction(Menuitem *mi, int i)
{
	if (mi->D.Pulldown.curopt == i) {
		LocalSetupState.Race[NetLocalHostsSlot] = mi->D.Pulldown.noptions - 1 - i;
		MultiClientUpdate(0);
	}
}
#endif

#if 0
/**
** Multiplayer client ready button
*/
static void MultiClientReady(void)
{
	Menu *menu;

	menu = FindMenu("menu-net-multi-client");
	menu->Items[2].Flags = MI_FLAGS_DISABLED;
	menu->Items[3].Flags = 0;
	menu->Items[CLIENT_RACE].Flags = MI_FLAGS_DISABLED;
	LocalSetupState.Ready[NetLocalHostsSlot] = 1;
	MultiClientUpdate(0);
}
#endif

#if 0
/**
** Multiplayer client not ready button
*/
static void MultiClientNotReady(void)
{
	Menu *menu;

	menu = FindMenu("menu-net-multi-client");
	menu->Items[3].Flags = MI_FLAGS_DISABLED;
	menu->Items[2].Flags = 0;
	menu->Items[CLIENT_RACE].Flags = 0;
	LocalSetupState.Ready[NetLocalHostsSlot] = 0;
	MultiClientUpdate(0);
}
#endif

/**
** Callback from netconnect loop in Client-Sync state:
** Compare local state with server's information
** and force update when changes have occured.
*/
void NetClientCheckLocalState(void)
{
	if (LocalSetupState.Ready[NetLocalHostsSlot] != ServerSetupState.Ready[NetLocalHostsSlot]) {
		NetLocalState = ccs_changed;
		return;
	}
	if (LocalSetupState.Race[NetLocalHostsSlot] != ServerSetupState.Race[NetLocalHostsSlot]) {
		NetLocalState = ccs_changed;
		return;
	}
	/* ADD HERE */
}

/**
** FIXME: docu
*/
int NetClientSelectScenario(void)
{
	char *cp;

	cp = strrchr(MenuMapFullPath, '/');
	if (cp) {
		strcpy(ScenSelectFileName, cp + 1);
		*cp = 0;
		strcpy(ScenSelectPath, MenuMapFullPath);
		*cp = '/';
	} else {
		strcpy(ScenSelectFileName, MenuMapFullPath);
		ScenSelectPath[0] = 0;
	}

	FreeMapInfo(&Map.Info);
	LoadStratagusMapInfo(MenuMapFullPath);
	return 0;
}

/**
** FIXME: docu
*/
void NetConnectForceDisplayUpdate(void)
{
//	MultiGamePlayerSelectorsUpdate(2);
}

#if 0
/**
** Update client menu to follow server menu.
*/
void NetClientUpdateState(void)
{
	Menu *menu;

	menu = FindMenu("menu-net-multi-client");

	GameRESAction(NULL, ServerSetupState.ResourcesOption);
	menu->Items[CLIENT_RESOURCE].D.Pulldown.curopt =
		ServerSetupState.ResourcesOption;

	GameUNSAction(NULL, ServerSetupState.UnitsOption);
	menu->Items[CLIENT_UNITS].D.Pulldown.curopt =
		ServerSetupState.UnitsOption;

	MultiGameFWSAction(NULL, ServerSetupState.FogOfWar);
	menu->Items[CLIENT_FOG_OF_WAR].D.Pulldown.curopt =
		ServerSetupState.FogOfWar;

	GameTSSAction(NULL, ServerSetupState.TilesetSelection);
	menu->Items[CLIENT_TILESET].D.Pulldown.curopt =
		ServerSetupState.TilesetSelection;

	GameGATAction(NULL, ServerSetupState.GameTypeOption);
	menu->Items[CLIENT_GAMETYPE].D.Pulldown.curopt =
		ServerSetupState.GameTypeOption;

	MultiClientUpdate(0);
	DebugPrint("NetClientUpdateState\n");
}
#endif

#if 0
/**
** Setup Editor Paths
*/
void SetupEditor(void)
{
	char *s;
	//
	//  Create a default path + map.
	//
	if (!*CurrentMapPath || *CurrentMapPath == '.' || *CurrentMapPath == '/') {
		strcpy(CurrentMapPath, DefaultMap);
	}

	//
	// Use the last path.
	//
	strcpy(ScenSelectPath, StratagusLibPath);
	if (*ScenSelectPath) {
		strcat(ScenSelectPath, "/");
	}
	strcat(ScenSelectPath, CurrentMapPath);
	if ((s = strrchr(ScenSelectPath, '/'))) {
		strcpy(ScenSelectFileName, s + 1);
		*s = '\0';
	}
	strcpy(ScenSelectDisplayPath, CurrentMapPath);
	if ((s = strrchr(ScenSelectDisplayPath, '/'))) {
		*s = '\0';
	} else {
		*ScenSelectDisplayPath = '\0';
	}

}
#endif


#if 0
/**
** Editor select cancel button callback
*/
static void EditorSelectCancel(void)
{
	//QuitToMenu = 1;
	Editor.Running = EditorNotRunning;
	//CloseMenu();
}
#endif

#if 0
/**
** Called from menu, for new editor map.
*/
static void EditorNewMap(void)
{
	Menu *menu;
	char width[10];
	char height[10];
	char description[36];

	EditorCancelled = 0;

	menu = FindMenu("menu-editor-new");
	menu->Items[2].D.Input.buffer = description;
	strcpy(description, "~!_");
	menu->Items[2].D.Input.nch = strlen(description) - 3;
	menu->Items[2].D.Input.maxch = 31;
	menu->Items[4].D.Input.buffer = width;
	strcpy(width, "128~!_");
	menu->Items[4].D.Input.nch = strlen(width) - 3;
	menu->Items[4].D.Input.maxch = 4;
	menu->Items[5].D.Input.buffer = height;
	strcpy(height, "128~!_");
	menu->Items[5].D.Input.nch = strlen(width) - 3;
	menu->Items[5].D.Input.maxch = 4;
	ProcessMenu("menu-editor-new", 1);

	if (EditorCancelled) {
		return;
	}

	description[strlen(description) - 3] = '\0';
	Map.Info.Description = new_strdup(description);
	Map.Info.MapWidth = atoi(width);
	Map.Info.MapHeight = atoi(height);

	Video.ClearScreen();

	*CurrentMapPath = '\0';

	GuiGameStarted = 1;
	//CloseMenu();
}
#endif

#if 0
/**
** Editor new map, size input box callback
*/
static void EditorNewMapSizeEnterAction(Menuitem * mi, int key)
{
	if (mi->D.Input.nch > 0
			&& !isdigit(mi->D.Input.buffer[mi->D.Input.nch - 1])) {
		strcpy(mi->D.Input.buffer + (--mi->D.Input.nch), "~!_");
	}
}
#endif

#if 0
/**
** Editor new map ok button
*/
static void EditorNewOk(void)
{
	Menu *menu;
	unsigned value1;
	unsigned value2;

	menu = CurrentMenu;
	value1 = atoi(menu->Items[4].D.Input.buffer);
	value2 = atoi(menu->Items[5].D.Input.buffer);

	if (value1 < 32 || value2 < 32) {
		if (value1 < 32) {
			sprintf(menu->Items[4].D.Input.buffer, "32~!_");
			menu->Items[4].D.Input.nch = strlen(menu->Items[4].D.Input.buffer) - 3;
		}
		if (value2 < 32) {
			sprintf(menu->Items[5].D.Input.buffer, "32~!_");
			menu->Items[5].D.Input.nch = strlen(menu->Items[5].D.Input.buffer) - 3;
		}
		ErrorMenu("Size smaller than 32");
	} else if (value1 > 1024 || value2 > 1024) {
		if (value1 == 0) {
			sprintf(menu->Items[4].D.Input.buffer, "1024~!_");
			menu->Items[4].D.Input.nch = strlen(menu->Items[4].D.Input.buffer) - 3;
		}
		if (value2 == 0) {
			sprintf(menu->Items[5].D.Input.buffer, "1024~!_");
			menu->Items[5].D.Input.nch = strlen(menu->Items[5].D.Input.buffer) - 3;
		}
		ErrorMenu("Size larger than 1024");
	} else if (value1 / 32 * 32 != value1 || value2/32*32 != value2) {
		if (value1 / 32 * 32 != value1) {
			sprintf(menu->Items[4].D.Input.buffer, "%d~!_", (value1 + 16) / 32 * 32);
			menu->Items[4].D.Input.nch = strlen(menu->Items[4].D.Input.buffer) - 3;
		}
		if (value2 / 32 * 32 != value2) {
			sprintf(menu->Items[5].D.Input.buffer, "%d~!_", (value2 + 16) / 32 * 32);
			menu->Items[5].D.Input.nch = strlen(menu->Items[5].D.Input.buffer) - 3;
		}
		ErrorMenu("Size must be a multiple of 32");
	}
	else {
		char tilemodel[PATH_MAX];

		sprintf(Map.TileModelsFileName, "scripts/tilesets/%s.lua",
				menu->Items[7].D.Pulldown.options[menu->Items[7].D.Pulldown.curopt]);
		sprintf(tilemodel, "%s/scripts/tilesets/%s.lua", StratagusLibPath,
				menu->Items[7].D.Pulldown.options[menu->Items[7].D.Pulldown.curopt]);
		LuaLoadFile(tilemodel);
		//CloseMenu();
	}
}
#endif

#if 0
/**
** Editor main load map menu
*/
static void EditorMainLoadMap(void)
{
	char *p;
	char *s;

	EditorCancelled = 0;
	ProcessMenu("menu-editor-main-load-map", 1);
	GetInfoFromSelectPath();

	if (EditorCancelled) {
		return;
	}

	Video.ClearScreen();

	if (ScenSelectPath[0]) {
		s = ScenSelectPath + strlen(ScenSelectPath);
		*s = '/';
		strcpy(s+1, ScenSelectFileName); // Final map name with path
		p = ScenSelectPath + strlen(StratagusLibPath) + 1;
		strcpy(CurrentMapPath, p);
		*s = '\0';
	} else {
		strcpy(CurrentMapPath, ScenSelectFileName);
	}
	
	GuiGameStarted = 1;
	//CloseMenu();
}
#endif

#if 0
/**
** Editor main load ok button
*/
static void EditorMainLoadOk(void)
{
	Menu *menu;
	Menuitem *mi;

	menu = CurrentMenu;
	mi = &menu->Items[1];
	if (ScenSelectPathName[0]) {
		strcat(ScenSelectPath, "/");
		strcat(ScenSelectPath, ScenSelectPathName);
		if (ScenSelectDisplayPath[0]) {
			strcat(ScenSelectDisplayPath, "/");
		}
		strcat(ScenSelectDisplayPath, ScenSelectPathName);
		EditorMainLoadLBInit(mi);
	} else if (ScenSelectFileName[0]) {
		//CloseMenu();
	}
}
#endif

#if 0
/**
** Editor main load cancel button
*/
static void EditorMainLoadCancel(void)
{
	char *s;

	EditorCancelled=1;

	//
	//  Use last selected map.
	//
	DebugPrint("Map   path: %s\n" _C_ CurrentMapPath);
	strcpy(ScenSelectPath, StratagusLibPath);
	if (*ScenSelectPath) {
		strcat(ScenSelectPath, "/");
	}
	strcat(ScenSelectPath, CurrentMapPath);
	if ((s = strrchr(ScenSelectPath, '/'))) {
		strcpy(ScenSelectFileName, s + 1);
		*s = '\0';
	}
	strcpy(ScenSelectDisplayPath, CurrentMapPath);
	if ((s = strrchr(ScenSelectDisplayPath, '/'))) {
		*s = '\0';
	} else {
		*ScenSelectDisplayPath = '\0';
	}

	DebugPrint("Start path: %s\n" _C_ ScenSelectPath);

	//CloseMenu();
}
#endif

#if 0
/**
**  Editor load map menu
*/
void EditorLoadMenu(void)
{
	char *p;
	char *s;

	EditorCancelled = 0;
	ProcessMenu("menu-editor-load", 1);
	GetInfoFromSelectPath();

	if (EditorCancelled) {
		return;
	}

	Video.ClearScreen();

	if (ScenSelectPath[0]) {
		s = ScenSelectPath + strlen(ScenSelectPath);
		*s = '/';
		strcpy(s + 1, ScenSelectFileName); // Final map name with path
		p = ScenSelectPath + strlen(StratagusLibPath) + 1;
		strcpy(CurrentMapPath, p);
		*s = '\0';
	} else {
		strcpy(CurrentMapPath, ScenSelectFileName);
	}

	Editor.MapLoaded = true;
	Editor.Running = EditorNotRunning;
	//CloseMenu();
}
#endif

#if 0
/**
** Editor main load ok button
*/
static void EditorLoadOk(void)
{
	Menu *menu;
	Menuitem *mi;

	menu = CurrentMenu;
	mi = &menu->Items[1];
	if (ScenSelectPathName[0]) {
		strcat(ScenSelectPath, "/");
		strcat(ScenSelectPath, ScenSelectPathName);
		if (ScenSelectDisplayPath[0]) {
			strcat(ScenSelectDisplayPath, "/");
		}
		strcat(ScenSelectDisplayPath, ScenSelectPathName);
		EditorMainLoadLBInit(mi);
	} else if (ScenSelectFileName[0]) {
		//CloseMenu();
	}
}
#endif

#if 0
/**
** Editor main load cancel button
*/
static void EditorLoadCancel(void)
{
	char *s;

	EditorCancelled = 1;

	//
	//  Use last selected map.
	//
	DebugPrint("Map   path: %s\n" _C_ CurrentMapPath);
	strcpy(ScenSelectPath, StratagusLibPath);
	if (*ScenSelectPath) {
		strcat(ScenSelectPath, "/");
	}
	strcat(ScenSelectPath, CurrentMapPath);
	if ((s = strrchr(ScenSelectPath, '/'))) {
		strcpy(ScenSelectFileName, s + 1);
		*s = '\0';
	}
	strcpy(ScenSelectDisplayPath, CurrentMapPath);
	if ((s = strrchr(ScenSelectDisplayPath, '/'))) {
		*s = '\0';
	} else {
		*ScenSelectDisplayPath = '\0';
	}

	DebugPrint("Start path: %s\n" _C_ ScenSelectPath);

	EditorEndMenu();
}
#endif

#if 0
/**
** Editor map properties menu
*/
static void EditorMapPropertiesMenu(void)
{
	Menu *menu;
	char description[36];
	char size[30];

	menu = FindMenu("menu-editor-map-properties");

	menu->Items[2].D.Input.buffer = description;
	strcpy(description, Map.Info.Description);
	strcat(description, "~!_");
	menu->Items[2].D.Input.nch = strlen(description) - 3;
	menu->Items[2].D.Input.maxch = 31;

	sprintf(size, "%d x %d", Map.Info.MapWidth, Map.Info.MapHeight);
	menu->Items[4].D.Text.text = NewStringDesc(size);
	menu->Items[6].D.Pulldown.defopt = 0;

	// FIXME: Remove the version pulldown
	menu->Items[8].D.Pulldown.defopt = 1;
	menu->Items[8].Flags = static_cast<unsigned int>(-1);

	ProcessMenu("menu-editor-map-properties", 1);
	FreeStringDesc(menu->Items[4].D.Text.text);
	delete menu->Items[4].D.Text.text;
	menu->Items[4].D.Text.text = NULL;
}
#endif

#if 0
/**
** Editor map properties ok button
*/
static void EditorMapPropertiesOk(void)
{
	Menu *menu;
	char *description;

	menu = CurrentMenu;

	description = menu->Items[2].D.Input.buffer;
	description[strlen(description)-3] = '\0';
	delete[] Map.Info.Description;
	Map.Info.Description = new_strdup(description);

	#if 0
	// MAPTODO
	// Change the terrain
	old = Map.Info.MapTerrain;
	if (old != menu->Items[6].D.Pulldown.curopt) {
		Map.Info.MapTerrain = menu->Items[6].D.Pulldown.curopt;
		delete[] Map.Info.MapTerrainName;
		Map.Info.MapTerrainName = new_strdup(TilesetWcNames[Map.Info.MapTerrain]);
		delete[] Map.TerrainName;
		Map.TerrainName = new_strdup(TilesetWcNames[Map.Info.MapTerrain]);
		Map.Tileset = Tilesets[Map.Info.MapTerrain];

		LoadTileset();
		SetPlayersPalette();
		PreprocessMap();
		LoadConstructions();
		LoadUnitTypes();
		LoadIcons();
		UpdateMinimapTerrain();
	}
	#endif


	EditorEndMenu();
}
#endif

#if 0
/**
** Editor player properties input box callback
*/
static void EditorPlayerPropertiesEnterAction(Menuitem *mi, int key)
{
	if (mi->D.Input.nch > 0 && !isdigit(mi->D.Input.buffer[mi->D.Input.nch - 1])) {
		strcpy(mi->D.Input.buffer + (--mi->D.Input.nch), "~!_");
	}
}
#endif

#if 0
	/// Player type conversion from internal fc to menu number
static int PlayerTypesFcToMenu[] = {
	0,
	0,
	4,
	5,
	1,
	0,
	2,
	3,
};
#endif

#if 0
	/// Player type conversion from menu to internal fc number
static int PlayerTypesMenuToFc[] = {
	PlayerPerson,
	PlayerComputer,
	PlayerRescuePassive,
	PlayerRescueActive,
	PlayerNeutral,
	PlayerNobody,
};
#endif

#if 0
/**
**  Convert player ai from internal fc number to menu number
**
**  @param ainame  Ai name
**  @param menu    Pulldown menu item
*/
static int PlayerSetAiToMenu(char *ainame, MenuitemPulldown *menu)
{
	int i;

	menu->defopt = 0;
	for (i = 0; i < menu->noptions; ++i) {
		if(!strcmp(menu->options[i], ainame)) {
			menu->defopt = i;
		}
	}
	DebugPrint("Invalid Ai number: %s\n" _C_ ainame);
	return i;
}
#endif

#if 0
/**
**  Get the ai ident from the pulldown menu
**
**  @param menu  Pulldown menu item
*/
static char *PlayerGetAiFromMenu(MenuitemPulldown *menu)
{
	return menu->options[menu->curopt];
}
#endif

#if 0
/**
** Edit player properties menu
*/
static void EditorPlayerPropertiesMenu(void)
{
	Menu *menu;
	char gold[PlayerMax][15];
	char lumber[PlayerMax][15];
	char oil[PlayerMax][15];
	int i;

	menu = FindMenu("menu-editor-player-properties");

#define RACE_POSITION 21
#define TYPE_POSITION 38
#define AI_POSITION 55
#define GOLD_POSITION 72
#define LUMBER_POSITION 89
#define OIL_POSITION 106

	for (i = 0; i < PlayerMax; ++i) {
		menu->Items[RACE_POSITION + i].D.Pulldown.defopt = Map.Info.PlayerSide[i];
		menu->Items[TYPE_POSITION + i].D.Pulldown.defopt = PlayerTypesFcToMenu[Map.Info.PlayerType[i]];
		PlayerSetAiToMenu(Players[i].AiName, &menu->Items[AI_POSITION + i].D.Pulldown);
		sprintf(gold[i], "%d~!_", Players[i].Resources[GoldCost]);
		sprintf(lumber[i], "%d~!_", Players[i].Resources[WoodCost]);
		sprintf(oil[i], "%d~!_", Players[i].Resources[OilCost]);
		menu->Items[GOLD_POSITION + i].D.Input.buffer = gold[i];
		menu->Items[GOLD_POSITION + i].D.Input.nch = strlen(gold[i]) - 3;
		menu->Items[GOLD_POSITION + i].D.Input.maxch = 7;
		menu->Items[LUMBER_POSITION + i].D.Input.buffer = lumber[i];
		menu->Items[LUMBER_POSITION + i].D.Input.nch = strlen(lumber[i]) - 3;
		menu->Items[LUMBER_POSITION + i].D.Input.maxch = 7;
		menu->Items[OIL_POSITION + i].D.Input.buffer = oil[i];
		menu->Items[OIL_POSITION + i].D.Input.nch = strlen(oil[i]) - 3;
		menu->Items[OIL_POSITION + i].D.Input.maxch = 7;
	}

	ProcessMenu("menu-editor-player-properties", 1);

	for (i = 0; i < PlayerMax; ++i) {
		Map.Info.PlayerSide[i] = menu->Items[RACE_POSITION + i].D.Pulldown.curopt;
		Map.Info.PlayerType[i] = PlayerTypesMenuToFc[menu->Items[TYPE_POSITION + i].D.Pulldown.curopt];
		strcpy(Players[i].AiName, 
				PlayerGetAiFromMenu(&menu->Items[AI_POSITION + i].D.Pulldown));
		Players[i].Resources[GoldCost] = atoi(gold[i]);
		Players[i].Resources[WoodCost] = atoi(lumber[i]);
		Players[i].Resources[OilCost] = atoi(oil[i]);
	}
}
#endif

/**
** Edit resource properties
*/
void EditorEditResource(void)
{
#if 0
	Menu *menu;
	char buf[13];
	char buf2[32];

	menu = FindMenu("menu-editor-edit-resource");

	sprintf(buf2, "Amount of %s:", DefaultResourceNames[UnitUnderCursor->Type->GivesResource]);
	menu->Items[0].D.Text.text = NewStringDesc(buf2);
	sprintf(buf, "%d~!_", UnitUnderCursor->ResourcesHeld);
	menu->Items[1].D.Input.buffer = buf;
	menu->Items[1].D.Input.nch = strlen(buf) - 3;
	menu->Items[1].D.Input.maxch = 6;
	ProcessMenu("menu-editor-edit-resource", 1);
	FreeStringDesc(menu->Items[0].D.Text.text);
	delete menu->Items[0].D.Text.text;
	menu->Items[0].D.Text.text = NULL;
#endif
}

#if 0
/**
** Key pressed in menu-editor-edit-resource
*/
static void EditorEditResourceEnterAction(Menuitem *mi,int key)
{
	if (mi->D.Input.nch > 0 && !isdigit(mi->D.Input.buffer[mi->D.Input.nch - 1])) {
		strcpy(mi->D.Input.buffer + (--mi->D.Input.nch), "~!_");
	} else if (key==10 || key==13) {
		EditorEditResourceOk();
	}
}
#endif

#if 0
/**
** Ok button from menu-editor-edit-resource
*/
static void EditorEditResourceOk(void)
{
	Menu *menu;
	unsigned value;

	menu = CurrentMenu;
	value = atoi(menu->Items[1].D.Input.buffer);
	if (value < 2500) {
		strcpy(menu->Items[1].D.Input.buffer, "2500~!_");
		menu->Items[1].D.Input.nch = 4;
		menu = FindMenu("menu-editor-error");
		menu->Items[1].D.Text.text = NewStringDesc("Must be greater than 2500");
	} else if (value > 655000) {
		strcpy(menu->Items[1].D.Input.buffer, "655000~!_");
		menu->Items[1].D.Input.nch = 6;
		menu = FindMenu("menu-editor-error");
		menu->Items[1].D.Text.text = NewStringDesc("Must be smaller than 655000");
	} else if (value / 2500 * 2500 != value) {
		value = (value + 1250)/ 2500 * 2500;
		sprintf(menu->Items[1].D.Input.buffer, "%d~!_", value);
		menu->Items[1].D.Input.nch = strlen(menu->Items[1].D.Input.buffer) - 3;
		menu = FindMenu("menu-editor-error");
		menu->Items[1].D.Text.text = NewStringDesc("Must be a multiple of 2500");
	} else {
		UnitUnderCursor->ResourcesHeld = value;
		GameMenuReturn();
		return;
	}
	ProcessMenu("menu-editor-error", 1);
	FreeStringDesc(menu->Items[1].D.Text.text);
	delete menu->Items[1].D.Text.text;
	menu->Items[1].D.Text.text = NULL;
}
#endif

#if 0
/**
** Cancel button from menu-editor-edit-resource
*/
static void EditorEditResourceCancel(void)
{
	GameMenuReturn();
}
#endif

/**
** Edit ai properties
*/
void EditorEditAiProperties(void)
{
#if 0
	Menu *menu;

	menu = FindMenu("menu-editor-edit-ai-properties");
	if (UnitUnderCursor->Active) {
		menu->Items[1].D.Checkbox.Checked = 1;
		menu->Items[3].D.Checkbox.Checked = 0;
	} else {
		menu->Items[1].D.Checkbox.Checked = 0;
		menu->Items[3].D.Checkbox.Checked = 1;
	}

	ProcessMenu("menu-editor-edit-ai-properties", 1);
#endif
}

#if 0
/**
** Active or Passive gem clicked in menu-editor-edit-ai-properties
*/
static void EditorEditAiPropertiesCheckbox(Menuitem *mi)
{
	if (&mi->Menu->Items[1] == mi) {
		mi->D.Checkbox.Checked = 1;
		mi->Menu->Items[3].D.Checkbox.Checked = 0;
	} else {
		mi->D.Checkbox.Checked = 1;
		mi->Menu->Items[1].D.Checkbox.Checked = 0;
	}
}
#endif

#if 0
/**
** Ok button from menu-editor-edit-ai-properties
*/
static void EditorEditAiPropertiesOk(void)
{
	Menu *menu;

	menu = CurrentMenu;
	if (menu->Items[1].D.Checkbox.Checked) {
		UnitUnderCursor->Active = 1;
	} else {
		UnitUnderCursor->Active = 0;
	}
	GameMenuReturn();
}
#endif

#if 0
/**
** Cancel button from menu-editor-edit-ai-properties
*/
static void EditorEditAiPropertiesCancel(void)
{
	GameMenuReturn();
}
#endif

#if 0
/**
** Save map from the editor
**
** @return 0 for success, -1 for error
*/
int EditorSaveMenu(void)
{
	Menu *menu;
	char path[PATH_MAX];
	char *s;
	char *p;
	int ret;

	ret = 0;
	menu = FindMenu("menu-editor-save");

	EditorCancelled = 0;

	strcpy(path, "~!_");
	menu->Items[3].D.Input.buffer = path;
	menu->Items[3].D.Input.maxch = PATH_MAX - 4;

	ProcessMenu("menu-editor-save", 1);

	if (!EditorCancelled) {
		if (Editor.WriteCompressedMaps && !strstr(path, ".gz")) {
			sprintf(path, "%s/%s.gz", ScenSelectPath, ScenSelectFileName);
		} else {
			sprintf(path, "%s/%s", ScenSelectPath, ScenSelectFileName);
		}
		if (EditorSaveMap(path) == -1) {
			ret = -1;
		} else {
			// Only change map path if we were able to save the map
			s = ScenSelectPath + strlen(ScenSelectPath);
			*s = '/';
			strcpy(s + 1, ScenSelectFileName); // Final map name with path
			p = ScenSelectPath + strlen(StratagusLibPath) + 1;
			strcpy(CurrentMapPath, p);
			*s = '\0';
		}
	}
	return ret;
}
#endif

#if 0
/**
** Editor save ok button
*/
static void EditorSaveOk(void)
{
	Menu *menu;
	Menuitem *mi;
	int i;

	menu = CurrentMenu;
	mi = &menu->Items[1];
	i = mi->D.Listbox.curopt;
	if (i < mi->D.Listbox.noptions) {
		if (mi->Menu->Items[3].D.Input.nch == 0 && ScenSelectPathName[0]) {
			strcat(ScenSelectPath, "/");
			strcat(ScenSelectPath, ScenSelectPathName);
			if (ScenSelectDisplayPath[0]) {
				strcat(ScenSelectDisplayPath, "/");
			}
			strcat(ScenSelectDisplayPath, ScenSelectPathName);
			EditorSaveLBInit(mi);
		} else {
			strcpy(ScenSelectFileName, menu->Items[3].D.Input.buffer); // Final map name
			ScenSelectFileName[strlen(ScenSelectFileName) - 3] = '\0';
			if (!strcasestr(ScenSelectFileName, ".smp")) {
				strcat(ScenSelectFileName, ".smp");
			}
			sprintf(TempPathBuf, "%s/%s.gz", ScenSelectPath, ScenSelectFileName);
			if (!access(TempPathBuf, F_OK)) {
				ProcessMenu("menu-editor-save-confirm", 1);
				if (EditorCancelled) {
					EditorCancelled = 0;
					return;
				}
			}
			EditorEndMenu();
		}
	}
}
#endif

#if 0
/**
** Editor save input callback
*/
static void EditorSaveEnterAction(Menuitem *mi, int key)
{
	Assert(mi->MiType == MiTypeInput);

	strncpy(ScenSelectFileName, mi->D.Input.buffer, mi->D.Input.nch);
	ScenSelectFileName[mi->D.Input.nch] = '\0';
	ScenSelectPathName[0] = '\0';
	SelectedFileExist = 0;
	if (mi->D.Input.nch != 0) {
		if (key == 10 || key == 13) {
			EditorSaveOk();
		}
	}
}
#endif

#if 0
/**
** Called from menu, to quit editor to menu.
**
** @todo Should check if modified file should be saved.
*/
static void EditorQuitToMenu(void)
{
	//QuitToMenu = 1;
	Editor.Running = EditorNotRunning;
	//CloseMenu();
	SelectedFileExist = 0;
	ScenSelectFileName[0] = '\0';
	ScenSelectPathName[0] = '\0';
}
#endif

/*----------------------------------------------------------------------------
--  Metaserver
----------------------------------------------------------------------------*/

#if 0
/**
** Start process network game setup menu (server).
** Internet game, register with meta server
*/
static void CreateInternetGameMenu(void)
{
	GuiGameStarted = 0;
	AddGameServer();
	ProcessMenu("menu-multi-setup", 1);
	if (GuiGameStarted) {
		GameMenuReturn();
	}

}
#endif

#if 0
/**
**  FIXME: docu
*/
static void MultiGameMasterReport(void)
{
// CloseMenu();

	ProcessMenu("metaserver-list", 1);
	if (GuiGameStarted) {
		GameMenuReturn();
	}

}
#endif

#if 0
/**
**  Menu for Mater Server Game list.
*/
static void ShowMetaServerList(void)
{
	//CloseMenu();

	GuiGameStarted = 0;
	ProcessMenu("metaserver-list", 1);
	if (GuiGameStarted) {
		GameMenuReturn();
	}
}
#endif

#if 0
/**
**  Multiplayer server menu init callback
**
**  Mohydine: Right now, because I find it simpler, the client is sending
**            n commands, one for each online game.
**  @todo: well, redo this :)
*/
static void MultiMetaServerGameSetupInit(Menu *menu)
{
	int i;
	int j;
	int k;
	int numparams;
	int nummenus;
	char *parameter;
	char *reply;
	char *port;

	SendMetaCommand("NumberOfGames", "");

	reply = NULL;
	// receive
	// check okay
	if (RecvMetaReply(&reply) == -1) {
		//TODO: Notify player that connection was aborted...
		nummenus = 1;
	} else {
		for (i = 0; i < 3; ++i) {
			GetMetaParameter(reply, 0, &parameter);
			nummenus = atoi(parameter);
			delete[] parameter;
			if (nummenus == 0) {
				RecvMetaReply(&reply);
			}
			else {
				break;
			}
		}

	}

	--nummenus;
	// Meta server only sends matching version
	// Only Displays games from Matching version

	i = 1;
	k = 0;
	numparams = 5; // TODO: To be changed if more params are sent

	// Retrieve list of online game from the meta server
	for (j = 4; j <= nummenus * (numparams + 1); j += numparams + 1) { // loop over the number of items in the menu
		// TODO: hard coded.
		// Check if connection to meta server is there.

		SendMetaCommand("GameNumber","%d\n",k + 1);
		i = RecvMetaReply(&reply);
		if (i == 0) {
			// fill the menus with the right info.
			menu->Items[j].D.Text.text = NULL;
			menu->Items[j + 1].D.Text.text = NULL;
			menu->Items[j + 2].D.Text.text = NULL;
			menu->Items[j + 3].D.Text.text = NULL;
			menu->Items[j + 4].D.Text.text = NULL;
			menu->Items[j + 5].Flags = MI_FLAGS_INVISIBLE;
		} else {
			GetMetaParameter(reply, 0, &parameter);       // Player Name
			menu->Items[j].D.Text.text = NewStringDesc(parameter);
			delete[] parameter;
			GetMetaParameter(reply, 3, &parameter);       // IP
			GetMetaParameter(reply, 4, &port);            // port
			sprintf(parameter, "%s:%s", parameter, port); // IP:Port
			menu->Items[j + 1].D.Text.text = NewStringDesc(parameter);
			delete[] parameter;
			delete[] port;
			GetMetaParameter(reply, 6, &parameter);
			menu->Items[j + 2].D.Text.text = NewStringDesc(parameter);
			delete[] parameter;
			GetMetaParameter(reply, 7, &parameter);
			menu->Items[j + 3].D.Text.text = NewStringDesc(parameter);
			delete[] parameter;
			GetMetaParameter(reply, 8, &parameter);
			menu->Items[j + 4].D.Text.text = NewStringDesc(parameter);
			menu->Items[j + 5].D.Checkbox.Checked = 0;
			delete[] parameter;
		}
		++k;
	}

	// Don't display slots not in use
	// FIXME: HardCoded Number of Items in list
	// 5 is the hardcoded value
	for (; j <= numparams * 5; j += numparams + 1) {
		// fill the menus with the right info.
		menu->Items[j].D.Text.text = NULL;
		menu->Items[j + 1].D.Text.text = NULL;
		menu->Items[j + 2].D.Text.text = NULL;
		menu->Items[j + 3].D.Text.text = NULL;
		menu->Items[j + 4].D.Text.text = NULL;
		menu->Items[j + 5].Flags = MI_FLAGS_DISABLED;
	}
}
#endif

#if 0
/**
**  Multiplayer server menu exit callback
*/
static void MultiMetaServerGameSetupExit(Menu *menu)
{
	int i;
	int j;
	int numparam;
	int nummenu;

	numparam = 5;
	nummenu = 6;
	for (j = 4; j <= numparam * nummenu; ++j) {
		for (i = 0; i < numparam; ++i) {
			FreeStringDesc(menu->Items[i + j].D.Text.text);
			delete menu->Items[i + j].D.Text.text;
			menu->Items[i + j].D.Text.text = NULL;
		}
	}
// CloseMenu();
// CloseMenu();
}
#endif

#if 0
/**
**  Action taken when a player select an online game
*/
static void SelectGameServer(Menuitem *mi)
{
	char server_host_buffer[64];
	char *port;
	int j;
	char *tmp;

	j = mi - mi->Menu->Items;
	mi->Menu->Items[j].D.Checkbox.Checked = 0;
	//CloseMenu();

	tmp = EvalString(mi->Menu->Items[j - 4].D.Text.text);
	strcpy(server_host_buffer, tmp);
	delete[] tmp;

	// Launch join directly
	if ((port = strchr(server_host_buffer, ':')) != NULL) {
		NetworkPort = atoi(port + 1);
		port[0] = 0;
	}

	// Now finally here is the address
// server_host_buffer[menu->Items[1].D.Input.nch] = 0;
	if (NetworkSetupServerAddress(server_host_buffer)) {
		NetErrorMenu("Unable to lookup host.");
		ProcessMenu("metaserver-list", 1);
		return;
	}
	NetworkInitClientConnect();
	if (!NetConnectRunning) {
		TerminateNetConnect();
		return;
	}

	delete[] NetworkArg;
	NetworkArg = new_strdup(server_host_buffer);

	// Here we really go...
	ProcessMenu("menu-net-connecting", 1);

	if (GuiGameStarted) {
		//CloseMenu();
	}
}
#endif

#if 0
/**
**  Action to add a game server on the meta-server.
*/
static void AddGameServer(void)
{
	// send message to meta server. meta server will detect IP address.
	// Meta-server will return "BUSY" if the list of online games is busy.

	SendMetaCommand("AddGame", "%s\n%d\n%s\n%s\n%s\n%s\n",
		"IP", NetworkPort, "Name", "Map", "Players", "Free");

	// FIXME: Get Reply from Queue
}
#endif

#if 0
/**
**  Action to add a game server on the meta-server.
*/
static void ChangeGameServer(void)
{
	int i;
	int freespots;
	int players;

	// send message to meta server. meta server will detect IP address.
	// Meta-server will return "ERR" if the list of online games is busy.

	freespots = 0;
	players = 0;
	for (i = 0; i < PlayerMax - 1; ++i) {
		if (Map.Info.PlayerType[i] == PlayerPerson) {
			++players;
		}
		if (ServerSetupState.CompOpt[i] == 0) {
			++freespots;
		}
	}
	SendMetaCommand("ChangeGame", "%s\n%s\n%d\n%d\n",
		"Name", ScenSelectFileName, players, freespots - 1);

	// FIXME: Get Reply from Queue
}
#endif

#if 0
/**
**  FIXME: docu
*/
static int MetaServerConnectError(void)
{
	Invalidate();
	NetErrorMenu("Cannot Connect to Meta-Server");
	return 0;
}
#endif

#if 0
/**
**  Close MetaServer connection
*/
static void MultiMetaServerClose(void)
{
	MetaClose();
	MetaServerInUse = 0;
	//CloseMenu();
}
#endif

//@}
