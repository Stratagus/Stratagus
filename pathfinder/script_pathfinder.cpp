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
/**@name ccl_pathfinder.c	-	pathfinder ccl functions. */
//
//	(c) Copyright 2000-2002 by Lutz Sammer, Fabrice Rossi, Latimerius.
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
#include "video.h"
#include "tileset.h"
#include "map.h"
#include "sound_id.h"
#include "unitsound.h"
#include "unittype.h"
#include "player.h"
#include "unit.h"
#include "ccl.h"
#include "pathfinder.h"

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

/**
**	Enable a*.
*/
#if defined(USE_GUILE) || defined(USE_SIOD)
local SCM CclAStar(SCM list)
{
    SCM value;
    int i;
    
    while( !gh_null_p(list) ) {
	value=gh_car(list);
	list=gh_cdr(list);
	if( gh_eq_p(value,gh_symbol2scm("fixed-unit-cost")) ) {
	    i=gh_scm2int(gh_car(list));
            list=gh_cdr(list);
	    if( i <=3 ) {
		PrintFunction();
		fprintf(stdout,"Fixed unit crossing cost must be strictly > 3\n");
	    } else {
		AStarFixedUnitCrossingCost=i;
	    }
	} else if( gh_eq_p(value,gh_symbol2scm("moving-unit-cost")) ) {
	    i=gh_scm2int(gh_car(list));
	    list=gh_cdr(list);
	    if( i<=3) {
		PrintFunction();
		fprintf(stdout,"Moving unit crossing cost must be strictly > 3\n");
	    } else {
		AStarMovingUnitCrossingCost=i;
	    }
	} else if( gh_eq_p(value,gh_symbol2scm("know-unseen-terrain")) ) {
	    AStarKnowUnknown=1;
	} else if( gh_eq_p(value,gh_symbol2scm("dont-know-unseen-terrain")) ) {
	    AStarKnowUnknown=0;
	} else if( gh_eq_p(value,gh_symbol2scm("unseen-terrain-cost")) ) {
	    i=gh_scm2int(gh_car(list));
	    if( i < 0 ) {
		PrintFunction();
		fprintf(stdout,"Unseen Terrain Cost must be non-negative\n");
	    } else {
		AStarUnknownTerrainCost=i;
	    }
	    list=gh_cdr(list);
	} else {
	    errl("Unsupported tag",value);
	}
    }
					  
    return SCM_UNSPECIFIED;
}
#elif defined(USE_LUA)
local int CclAStar(lua_State* l)
{
    const char* value;
    int i;
    int j;
    int args;

    args = lua_gettop(l);
    for (j = 0; j < args; ++j) {
	if (!lua_isstring(l, j + 1)) {
	    lua_pushstring(l, "incorrect argument");
	    lua_error(l);
	}
	value = lua_tostring(l, j + 1);
	if (!strcmp(value, "fixed-unit-cost")) {
	    ++j;
	    if (!lua_isnumber(l, j + 1)) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	    }
	    i = lua_tonumber(l, j + 1);
	    if (i <= 3) {
		PrintFunction();
		fprintf(stdout, "Fixed unit crossing cost must be strictly > 3\n");
	    } else {
		AStarFixedUnitCrossingCost = i;
	    }
	} else if (!strcmp(value, "moving-unit-cost")) {
	    ++j;
	    if (!lua_isnumber(l, j + 1)) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	    }
	    i = lua_tonumber(l, j + 1);
	    if (i <= 3) {
		PrintFunction();
		fprintf(stdout, "Moving unit crossing cost must be strictly > 3\n");
	    } else {
		AStarMovingUnitCrossingCost = i;
	    }
	} else if (!strcmp(value, "know-unseen-terrain")) {
	    AStarKnowUnknown = 1;
	} else if (!strcmp(value, "dont-know-unseen-terrain")) {
	    AStarKnowUnknown = 0;
	} else if (!strcmp(value, "unseen-terrain-cost")) {
	    ++j;
	    if (!lua_isnumber(l, j + 1)) {
		lua_pushstring(l, "incorrect argument");
		lua_error(l);
	    }
	    i = lua_tonumber(l, j + 1);
	    if (i < 0) {
		PrintFunction();
		fprintf(stdout, "Unseen Terrain Cost must be non-negative\n");
	    } else {
		AStarUnknownTerrainCost = i;
	    }
	} else {
	    lua_pushfstring(l, "Unsupported tag: %s", value);
	    lua_error(l);
	}
    }

    return 0;
}
#endif

#ifdef HIERARCHIC_PATHFINDER
local SCM CclPfHierShowRegIds (SCM flag)
{
    PfHierShowRegIds = gh_scm2bool (flag);
    return SCM_UNSPECIFIED;
}

local SCM CclPfHierShowGroupIds (SCM flag)
{
    PfHierShowGroupIds = gh_scm2bool (flag);
    return SCM_UNSPECIFIED;
}
#else
#if defined(USE_GUILE) || defined(USE_SIOD)
local SCM CclPfHierShowRegIds (SCM flag __attribute__((unused)))
{
    return SCM_UNSPECIFIED;
}

local SCM CclPfHierShowGroupIds (SCM flag __attribute__((unused)))
{
    return SCM_UNSPECIFIED;
}

#ifdef MAP_REGIONS
global void MapSplitterDebug(void);

local SCM CclDebugRegions(SCM flag __attribute__((unused)))
{
    MapSplitterDebug();
    return SCM_UNSPECIFIED;
}
#endif // MAP_REGIONS

#elif defined(USE_LUA)
#endif
#endif


/**
**	Register CCL features for pathfinder.
*/
global void PathfinderCclRegister(void)
{
#if defined(USE_GUILE) || defined(USE_SIOD)
    gh_new_procedureN("a-star",CclAStar);
#ifdef MAP_REGIONS
    gh_new_procedureN("debug-regions",CclDebugRegions);
#endif // MAP_REGIONS
    gh_new_procedure1_0 ("pf-show-regids!", CclPfHierShowRegIds);
    gh_new_procedure1_0 ("pf-show-groupids!", CclPfHierShowGroupIds);
#elif defined(USE_LUA)
    lua_register(Lua, "AStar",CclAStar);
//    lua_register(Lua, "PfShowRegids", CclPfHierShowRegIds);
//    lua_register(Lua, "PfShowGroupids", CclPfHierShowGroupIds);
#endif
}

//@}
