//       _________ __                 __                               
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/ 
//  ______________________                           ______________________
//			  T H E   W A R   B E G I N S
//	   Stratagus - A free fantasy real time strategy game engine
//
/**@name ccl_player.c	-	The player ccl functions. */
//
//	(c) Copyright 2001-2003 by Lutz Sammer
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; version 2 dated June, 1991.
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
//	$Id$

//@{

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stratagus.h"
#include "player.h"
#include "ccl.h"
#include "ai.h"
#include "actions.h"
#include "commands.h"

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

/**
**	Get a player pointer
**
**	@param value	Player slot number.
**
**	@return		The player pointer
*/
local Player* CclGetPlayer(SCM value)
{
    return &Players[gh_scm2int(value)];
}

/**
**	Parse the player configuration.
**
**	@param list	Tagged list of all informations.
*/
local SCM CclPlayer(SCM list)
{
    SCM value;
    SCM sublist;
    Player* player;
    int i;
    char* str;

    i = gh_scm2int(gh_car(list));
    player = &Players[i];
    if (NumPlayers <= i) {
	NumPlayers = i + 1;
    }
    player->Player = i;
    player->Color = PlayerColors[i];
    if (!(player->Units = (Unit**)calloc(UnitMax, sizeof(Unit*)))) {
	DebugLevel0("Not enough memory to create player %d.\n" _C_ i);

	return SCM_UNSPECIFIED;
    }
    list = gh_cdr(list);

    //
    //	Parse the list:	(still everything could be changed!)
    //
    while (!gh_null_p(list)) {
	value = gh_car(list);
	list = gh_cdr(list);

	if (gh_eq_p(value, gh_symbol2scm("name"))) {
	    player->Name = gh_scm2newstr(gh_car(list), NULL);
	    list = gh_cdr(list);
	} else if (gh_eq_p(value, gh_symbol2scm("type"))) {
	    value = gh_car(list);
	    list = gh_cdr(list);
	    if (gh_eq_p(value, gh_symbol2scm("neutral"))) {
		player->Type = PlayerNeutral;
	    } else if (gh_eq_p(value, gh_symbol2scm("nobody"))) {
		player->Type = PlayerNobody;
	    } else if (gh_eq_p(value, gh_symbol2scm("computer"))) {
		player->Type = PlayerComputer;
	    } else if (gh_eq_p(value, gh_symbol2scm("person"))) {
		player->Type = PlayerPerson;
	    } else if (gh_eq_p(value, gh_symbol2scm("rescue-passive"))) {
		player->Type = PlayerRescuePassive;
	    } else if (gh_eq_p(value, gh_symbol2scm("rescue-active"))) {
		player->Type = PlayerRescueActive;
	    } else {
	       // FIXME: this leaves a half initialized player
	       errl("Unsupported tag", value);
	    }
	} else if (gh_eq_p(value, gh_symbol2scm("race"))) {
	    str = gh_scm2newstr(gh_car(list),NULL);
	    for (i = 0; i < PlayerRaces.Count; ++i) {
		if (!strcmp(str, PlayerRaces.Name[i])) {
		    player->RaceName = PlayerRaces.Name[i];
		    player->Race = i;
		    break;
		}
	    }
	    free(str);
	    if (i == PlayerRaces.Count) {
	       // FIXME: this leaves a half initialized player
	       errl("Unsupported race", gh_car(list));
	    }
	    list = gh_cdr(list);
	} else if (gh_eq_p(value, gh_symbol2scm("ai"))) {
	    player->AiNum = gh_scm2int(gh_car(list));
	    list = gh_cdr(list);
	} else if (gh_eq_p(value, gh_symbol2scm("team"))) {
	    player->Team = gh_scm2int(gh_car(list));
	    list = gh_cdr(list);
	} else if (gh_eq_p(value, gh_symbol2scm("enemy"))) {
	    str = gh_scm2newstr(gh_car(list), NULL);
	    list = gh_cdr(list);
	    for (i = 0; i < PlayerMax && *str; ++i, ++str) {
		if (*str == '-' || *str == '_' || *str == ' ') {
		    player->Enemy &= ~(1 << i);
		} else {
		    player->Enemy |= (1 << i);
		}
	    }
	} else if (gh_eq_p(value, gh_symbol2scm("allied"))) {
	    str = gh_scm2newstr(gh_car(list), NULL);
	    list = gh_cdr(list);
	    for (i = 0; i < PlayerMax && *str; ++i, ++str) {
		if (*str == '-' || *str == '_' || *str == ' ') {
		    player->Allied &= ~(1 << i);
		} else {
		    player->Allied |= (1 << i);
		}
	    }
	} else if (gh_eq_p(value, gh_symbol2scm("shared-vision"))) {
	    str = gh_scm2newstr(gh_car(list), NULL);
	    list = gh_cdr(list);
	    for (i = 0; i < PlayerMax && *str; ++i, ++str) {
		if (*str == '-' || *str == '_' || *str == ' ') {
		    player->SharedVision &= ~(1 << i);
		} else {
		    player->SharedVision |= (1 << i);
		}
	    }
	} else if (gh_eq_p(value, gh_symbol2scm("start"))) {
	    value = gh_car(list);
	    list = gh_cdr(list);
	    player->StartX = gh_scm2int(gh_car(value));
	    player->StartY = gh_scm2int(gh_cadr(value));
	} else if (gh_eq_p(value, gh_symbol2scm("resources"))) {
	    sublist = gh_car(list);
	    list = gh_cdr(list);
	    while (!gh_null_p(sublist)) {
		value = gh_car(sublist);
		sublist = gh_cdr(sublist);

		for (i = 0; i < MaxCosts; ++i) {
		    if (gh_eq_p(value, gh_symbol2scm((char*)DefaultResourceNames[i]))) {
			player->Resources[i] = gh_scm2int(gh_car(sublist));
			break;
		    }
		}
		if (i == MaxCosts) {
		   // FIXME: this leaves a half initialized player
		   errl("Unsupported tag", value);
		}
		sublist = gh_cdr(sublist);
	    }
	} else if (gh_eq_p(value, gh_symbol2scm("incomes"))) {
	    sublist = gh_car(list);
	    list = gh_cdr(list);
	    while (!gh_null_p(sublist)) {
		value = gh_car(sublist);
		sublist = gh_cdr(sublist);

		for (i = 0; i < MaxCosts; ++i) {
		    if (gh_eq_p(value, gh_symbol2scm((char*)DefaultResourceNames[i]))) {
			player->Incomes[i] = gh_scm2int(gh_car(sublist));
			break;
		    }
		}
		if (i == MaxCosts) {
		   // FIXME: this leaves a half initialized player
		   errl("Unsupported tag", value);
		}
		sublist = gh_cdr(sublist);
	    }
	} else if (gh_eq_p(value, gh_symbol2scm("ai-enabled"))) {
	    player->AiEnabled = 1;
	} else if (gh_eq_p(value, gh_symbol2scm("ai-disabled"))) {
	    player->AiEnabled = 0;
	} else if (gh_eq_p(value, gh_symbol2scm("food"))) {
	    player->Food = gh_scm2int(gh_car(list));
	    list = gh_cdr(list);
	} else if (gh_eq_p(value, gh_symbol2scm("food-unit-limit"))) {
	    player->FoodUnitLimit = gh_scm2int(gh_car(list));
	    list = gh_cdr(list);
	} else if (gh_eq_p(value, gh_symbol2scm("building-limit"))) {
	    player->BuildingLimit = gh_scm2int(gh_car(list));
	    list = gh_cdr(list);
	} else if (gh_eq_p(value, gh_symbol2scm("total-unit-limit"))) {
	    player->TotalUnitLimit = gh_scm2int(gh_car(list));
	    list = gh_cdr(list);
	} else if (gh_eq_p(value, gh_symbol2scm("score"))) {
	    player->Score = gh_scm2int(gh_car(list));
	    list = gh_cdr(list);
	} else if (gh_eq_p(value, gh_symbol2scm("total-units"))) {
	    player->TotalUnits = gh_scm2int(gh_car(list));
	    list = gh_cdr(list);
	} else if (gh_eq_p(value, gh_symbol2scm("total-buildings"))) {
	    player->TotalBuildings = gh_scm2int(gh_car(list));
	    list = gh_cdr(list);
	} else if (gh_eq_p(value, gh_symbol2scm("total-razings"))) {
	    player->TotalRazings = gh_scm2int(gh_car(list));
	    list = gh_cdr(list);
	} else if (gh_eq_p(value, gh_symbol2scm("total-kills"))) {
	    player->TotalKills = gh_scm2int(gh_car(list));
	    list = gh_cdr(list);
	} else if (gh_eq_p(value, gh_symbol2scm("total-resources"))) {
	    sublist = gh_car(list);
	    list = gh_cdr(list);
	    i = gh_length(sublist);
	    if (i != MaxCosts) {
		fprintf(stderr, "Wrong number of total-resources %d\n", i);
	    }
	    i = 0;
	    while (!gh_null_p(sublist)) {
		if (i < MaxCosts) {
		    player->TotalResources[i] = gh_scm2int(gh_car(sublist));
		}
		sublist = gh_cdr(sublist);
		++i;
	    }
	    player->TotalUnits = gh_scm2int(gh_car(list));
	} else if (gh_eq_p(value, gh_symbol2scm("timers"))) {
	    sublist = gh_car(list);
	    list = gh_cdr(list);
	    i = gh_length(sublist);
	    if (i != UpgradeMax) {
		fprintf(stderr, "Wrong upgrade timer length %d\n", i);
	    }

	    i = 0;
	    while (!gh_null_p(sublist)) {
		if (i < UpgradeMax) {
		    player->UpgradeTimers.Upgrades[i] =
			gh_scm2int(gh_car(sublist));
		}
		sublist = gh_cdr(sublist);
		++i;
	    }
	} else {
	   // FIXME: this leaves a half initialized player
	   errl("Unsupported tag", value);
	}
    }

    return SCM_UNSPECIFIED;
}

/**
**	Change unit owner
**
**	@param pos1	 top left tile
**	@param pos2	 bottom right tile
**	@param oldplayer old player number
**	@param newplayer new player number
**/
local SCM CclChangeUnitsOwner(SCM pos1, SCM pos2, SCM oldplayer, SCM newplayer)
{
    Unit* table[UnitMax];
    int n;
    int oldp;
    int newp;
    
    n = SelectUnits(gh_scm2int(gh_car(pos1)), gh_scm2int(gh_cadr(pos1)),
	gh_scm2int(gh_car(pos2)), gh_scm2int(gh_cadr(pos2)), table);
    oldp = gh_scm2int(oldplayer);
    newp = gh_scm2int(newplayer);
    while (n) {
        if (table[n - 1]->Player->Player == oldp) {
	    ChangeUnitOwner(table[n - 1], &Players[newp]);
	}
	--n;
    }
    return SCM_UNSPECIFIED;
}

/**
**	Get ThisPlayer.
**
**	@return		This player number.
*/
local SCM CclGetThisPlayer(void)
{
    return gh_int2scm(ThisPlayer - Players);
}

/**
**	Set ThisPlayer.
**
**	@param plynr	This player number.
*/
local SCM CclSetThisPlayer(SCM plynr)
{
    ThisPlayer = &Players[gh_scm2int(plynr)];

    return plynr;
}

/**
**	Set player unit limit.
**
**	@param limit	Unit limit.
*/
local SCM CclSetAllPlayersFoodUnitLimit(SCM limit)
{
    int i;

    for (i = 0; i < PlayerMax; ++i) {
	Players[i].FoodUnitLimit = gh_scm2int(limit);
    }

    return limit;
}

/**
**	Set player unit limit.
**
**	@param limit	Unit limit.
*/
local SCM CclSetAllPlayersBuildingLimit(SCM limit)
{
    int i;

    for (i = 0; i < PlayerMax; ++i) {
	Players[i].BuildingLimit = gh_scm2int(limit);
    }

    return limit;
}

/**
**	Set player unit limit.
**
**	@param limit	Unit limit.
*/
local SCM CclSetAllPlayersTotalUnitLimit(SCM limit)
{
    int i;

    for (i = 0; i < PlayerMax; ++i) {
	Players[i].TotalUnitLimit = gh_scm2int(limit);
    }

    return limit;
}

/**
**	Change the diplomacy from player to another player.
**
**	@param player	Player to change diplomacy.
**	@param opponent	Player number to change.
**	@param state	To which state this should be changed.
**
**	@return		FIXME: should return old state.
**
**	@todo FIXME: should return old state.
*/
local SCM CclSetDiplomacy(SCM player, SCM state, SCM opponent)
{
    int plynr;

#if 0
    Player* base;

    base = CclGetPlayer(player);
    plynr = gh_scm2int(opponent);

    if (gh_eq_p(state, gh_symbol2scm("allied"))) {
	base->Enemy &= ~(1 << plynr);
	base->Allied |= 1 << plynr;
    } else if (gh_eq_p(state, gh_symbol2scm("neutral"))) {
	base->Enemy &= ~(1 << plynr);
	base->Allied &= ~(1 << plynr);
    } else if (gh_eq_p(state, gh_symbol2scm("crazy"))) {
	base->Enemy |= 1 << plynr;
	base->Allied |= 1 << plynr;
    } else if (gh_eq_p(state, gh_symbol2scm("enemy"))) {
	base->Enemy |= 1 << plynr;
	base->Allied &= ~(1 << plynr);
    }

#else
    int base;

    base = gh_scm2int(player);
    plynr = gh_scm2int(opponent);

    // FIXME: must send over network!!

    if (gh_eq_p(state, gh_symbol2scm("allied"))) {
	SendCommandDiplomacy(base, DiplomacyAllied, plynr);
    } else if (gh_eq_p(state, gh_symbol2scm("neutral"))) {
	SendCommandDiplomacy(base, DiplomacyNeutral, plynr);
    } else if (gh_eq_p(state, gh_symbol2scm("crazy"))) {
	SendCommandDiplomacy(base, DiplomacyCrazy, plynr);
    } else if (gh_eq_p(state, gh_symbol2scm("enemy"))) {
	SendCommandDiplomacy(base, DiplomacyEnemy, plynr);
    }

#endif

    // FIXME: we can return the old state
    return SCM_UNSPECIFIED;
}

/**
**	Change the diplomacy from ThisPlayer to another player.
**
**	@param state	To which state this should be changed.
**	@param player	Player number to change.
*/
local SCM CclDiplomacy(SCM state, SCM player)
{
    return CclSetDiplomacy(gh_int2scm(ThisPlayer->Player), state, player);
}

/**
**	Change the shared vision from player to another player.
**
**	@param player	Player to change shared vision.
**	@param opponent	Player number to change.
**	@param state	To which state this should be changed.
**
**	@return		FIXME: should return old state.
**
**	@todo FIXME: should return old state.
*/
local SCM CclSetSharedVision(SCM player, SCM state, SCM opponent)
{
    int plynr;
    int base;
    int shared;

    base = gh_scm2int(player);
    shared = gh_scm2bool(state);
    plynr = gh_scm2int(opponent);

    SendCommandSharedVision(base, shared, plynr);

    // FIXME: we can return the old state
    return SCM_UNSPECIFIED;
}

/**
**	Change the shared vision from ThisPlayer to another player.
**
**	@param state	To which state this should be changed.
**	@param player	Player number to change.
*/
local SCM CclSharedVision(SCM state, SCM player)
{
    return CclSetSharedVision(gh_int2scm(ThisPlayer->Player), state, player);
}

/**
**	Define race names
**
**	@param list	List of all races.
*/
local SCM CclDefineRaceNames(SCM list)
{
    SCM sublist;
    SCM value;
    int i;

    PlayerRaces.Count = 0;
    while (!gh_null_p(list)) {
	value = gh_car(list);
	list = gh_cdr(list);

	if (gh_eq_p(value, gh_symbol2scm("race"))) {
	    sublist = gh_car(list);
	    list = gh_cdr(list);
	    i = PlayerRaces.Count++;
	    PlayerRaces.Race[i] = 0;
	    PlayerRaces.Name[i] = NULL;
	    PlayerRaces.Display[i] = NULL;
	    PlayerRaces.Visible[i] = 0;
	    while (!gh_null_p(sublist)) {
		value = gh_car(sublist);
		sublist = gh_cdr(sublist);
		if (gh_eq_p(value, gh_symbol2scm("race"))) {
		    PlayerRaces.Race[i] = gh_scm2int(gh_car(sublist));
		    sublist = gh_cdr(sublist);
		} else if (gh_eq_p(value, gh_symbol2scm("name"))) {
		    PlayerRaces.Name[i] = gh_scm2newstr(gh_car(sublist), NULL);
		    sublist = gh_cdr(sublist);
		} else if (gh_eq_p(value, gh_symbol2scm("display"))) {
		    PlayerRaces.Display[i] = gh_scm2newstr(gh_car(sublist), NULL);
		    sublist = gh_cdr(sublist);
		} else if (gh_eq_p(value, gh_symbol2scm("visible"))) {
		    PlayerRaces.Visible[i] = 1;
		} else {
		    errl("Unsupported tag", value);
		}
	    }
	} else {
	    errl("Unsupported tag", value);
	}
    }

    return SCM_UNSPECIFIED;
}

/**
**	Make new player colors
*/
local SCM CclNewPlayerColors(void)
{
    SetPlayersPalette();

    return SCM_UNSPECIFIED;
}

// ----------------------------------------------------------------------------

/**
**	Get player resources.
**
**	@param player	Player
**	@param resource	Resource name
**
**	@return		Player resource
*/
local SCM CclGetPlayerResource(SCM player, SCM resource)
{
    int i;
    Player* plyr;
    char* res;
    SCM ret;

    plyr = CclGetPlayer(player);
    res = gh_scm2newstr(resource, NULL);

    for (i = 0; i < MaxCosts; ++i) {
	if (!strcmp(res, DefaultResourceNames[i])) {
	    break;
	}
    }
    if (i == MaxCosts) {
       // FIXME: this leaves a half initialized player
       errl("Invalid resource", resource);
    }
    ret = gh_int2scm(plyr->Resources[i]);
    free(res);
    return ret;
}

/**
**	Set player resource.
**
**	@param list	Resource list
*/
local SCM CclSetPlayerResource(SCM list)
{
    int i;
    Player* player;
    SCM value;

    player = CclGetPlayer(gh_car(list));
    list = gh_cdr(list);
    while (!gh_null_p(list)) {
	value = gh_car(list);
	list = gh_cdr(list);
	for (i = 0; i < MaxCosts; ++i) {
	    if (gh_eq_p(value, gh_symbol2scm((char*)DefaultResourceNames[i]))) {
		break;
	    }
	}
	if (i == MaxCosts) {
	   // FIXME: this leaves a half initialized player
	   errl("Unsupported tag", value);
	}
	value = gh_car(list);
	list = gh_cdr(list);
	player->Resources[i] = gh_scm2int(value);
    }
    return SCM_UNSPECIFIED;
}

// ----------------------------------------------------------------------------

/**
**	Register CCL features for players.
*/
global void PlayerCclRegister(void)
{
    gh_new_procedureN("player", CclPlayer);
    gh_new_procedure4_0("change-units-owner", CclChangeUnitsOwner);
    gh_new_procedure0_0("get-this-player", CclGetThisPlayer);
    gh_new_procedure1_0("set-this-player!", CclSetThisPlayer);

    gh_new_procedure1_0("set-all-players-food-unit-limit!",
	CclSetAllPlayersFoodUnitLimit);
    gh_new_procedure1_0("set-all-players-building-limit!",
	CclSetAllPlayersBuildingLimit);
    gh_new_procedure1_0("set-all-players-total-unit-limit!",
	CclSetAllPlayersTotalUnitLimit);

    gh_new_procedure3_0("set-diplomacy!", CclSetDiplomacy);
    gh_new_procedure2_0("diplomacy", CclDiplomacy);
    gh_new_procedure3_0("set-shared-vision!", CclSetSharedVision);
    gh_new_procedure2_0("shared-vision", CclSharedVision);

    gh_new_procedureN("define-race-names", CclDefineRaceNames);

    gh_new_procedure0_0("new-colors", CclNewPlayerColors);

    // player member access functions
    gh_new_procedure2_0("get-player-resource", CclGetPlayerResource);
    gh_new_procedureN("set-player-resource!", CclSetPlayerResource);
}

//@}
