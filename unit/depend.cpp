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
/**@name depend.c	-	The units/upgrade dependencies */
//
//	(c) Copyright 2000-2003 by Vladi Belperchinov-Shabanski, Lutz Sammer,
//	                           and Jimmy Salmon
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
--		Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stratagus.h"
#include "upgrade_structs.h"
#include "upgrade.h"
#include "depend.h"
#include "player.h"
#include "ccl.h"

/*----------------------------------------------------------------------------
--		Variables
----------------------------------------------------------------------------*/

	/// All dependencies hash
local DependRule* DependHash[101];

/*----------------------------------------------------------------------------
--		Functions
----------------------------------------------------------------------------*/

/**
**		Add a new dependency. If already exits append to and rule.
**
**		@param target		Target of the dependency.
**		@param required		Requirement of the dependency.
**		@param count		Amount of the required needed.
**		@param or_flag		Start of or rule.
*/
global void AddDependency(const char* target, const char* required, int count,
	int or_flag)
{
	DependRule rule;
	DependRule* node;
	DependRule* temp;
	int hash;

	//
	//		Setup structure.
	//
	if (!strncmp(target, "unit-", 5)) {
		// target string refers to unit-xxx
		rule.Type = DependRuleUnitType;
		rule.Kind.UnitType = UnitTypeByIdent(target);
	} else if (!strncmp(target, "upgrade-", 8)) {
		// target string refers to upgrade-XXX
		rule.Type = DependRuleUpgrade;
		rule.Kind.Upgrade = UpgradeByIdent(target);
	} else {
		DebugLevel0Fn("dependency target `%s' should be unit-type or upgrade\n" _C_
			target);
		return;
	}
	hash = (int)(long)rule.Kind.UnitType % (sizeof(DependHash) / sizeof(*DependHash));

	//
	//		Find correct hash slot.
	//
	if ((node = DependHash[hash])) {		// find correct entry
		while (node->Type != rule.Type ||
				node->Kind.Upgrade != rule.Kind.Upgrade) {
			if (!node->Next) {				// end of list
				temp = malloc(sizeof(DependRule));
				temp->Next = NULL;
				temp->Rule = NULL;
				temp->Type = rule.Type;
				temp->Kind = rule.Kind;
				node->Next = temp;
				node = temp;
				break;
			}
			node = node->Next;
		}
	} else {								// create new slow
		node = malloc(sizeof(DependRule));
		node->Next = NULL;
		node->Rule = NULL;
		node->Type = rule.Type;
		node->Kind = rule.Kind;
		DependHash[hash] = node;
	}

	//
	//		Adjust count.
	//
	if (count < 0 || count > 255) {
		DebugLevel0Fn("wrong count `%d' range 0 .. 255\n" _C_ count);
		count = 255;
	}

	temp = malloc(sizeof(DependRule));
	temp->Rule = NULL;
	temp->Next = NULL;
	temp->Count = count;
	//
	//		Setup structure.
	//
	if (!strncmp(required, "unit-", 5)) {
		// required string refers to unit-xxx
		temp->Type = DependRuleUnitType;
		temp->Kind.UnitType = UnitTypeByIdent(required);
	} else if (!strncmp(required, "upgrade-", 8)) {
		// required string refers to upgrade-XXX
		temp->Type = DependRuleUpgrade;
		temp->Kind.Upgrade = UpgradeByIdent(required);
	} else {
		DebugLevel0Fn("dependency required `%s' should be unit-type or upgrade\n" _C_
			required);
		free(temp);
		return;
	}

	if (or_flag) {
			// move rule to temp->next
		temp->Next = node->Rule;				// insert rule
		node->Rule = temp;
	} else {
			// move rule to temp->rule
		temp->Rule = node->Rule;				// insert rule

		// also Link temp to old "or" list
		if (node->Rule) {
			temp->Next = node->Rule->Next;
		}

		node->Rule = temp;
	}

#ifdef neverDEBUG
	printf("New rules are :");
	node = node->Rule;
	while (node) {
			temp = node;
		while (temp){
				printf("temp->Kind.UnitType=%p ", temp->Kind.UnitType);
			temp = temp->Rule;
		}
		node = node->Next;
		printf("\n or ... ");
	}
	printf("\n");
#endif
}

/**
**		Check if this upgrade or unit is available.
**
**		@param player		For this player available.
**		@param target		Unit or Upgrade.
**
**		@return				True if available, false otherwise.
*/
global int CheckDependByIdent(const Player* player, const char* target)
{
	DependRule rule;
	const DependRule* node;
	const DependRule* temp;
	int i;

	//
	//		first have to check, if target is allowed itself
	//
	if (!strncmp(target, "unit-", 5)) {
		// target string refers to unit-XXX
		rule.Kind.UnitType = UnitTypeByIdent(target);
		if (UnitIdAllowed(player, rule.Kind.UnitType->Type) == 0) {
			return 0;
		}
		rule.Type = DependRuleUnitType;
	} else if (!strncmp(target, "upgrade-", 8)) {
		// target string refers to upgrade-XXX
		rule.Kind.Upgrade = UpgradeByIdent(target);
		if (UpgradeIdAllowed(player, rule.Kind.Upgrade - Upgrades) != 'A') {
			return 0;
		}
		rule.Type = DependRuleUpgrade;
	} else {
		DebugLevel0Fn("target `%s' should be unit-type or upgrade\n" _C_ target);
		return 0;
	}

	//
	//		Find rule
	//
	i = (int)(long)rule.Kind.UnitType % (sizeof(DependHash) / sizeof(*DependHash));

	if ((node = DependHash[i])) {		// find correct entry
		while (node->Type != rule.Type ||
				node->Kind.Upgrade != rule.Kind.Upgrade) {
			if (!node->Next) {				// end of list
				return 1;
			}
			node = node->Next;
		}
	} else {
		return 1;
	}

	//
	//		Prove the rules
	//
	node = node->Rule;

	while (node) {
		temp = node;
		while (temp) {
			switch (temp->Type) {
				case DependRuleUnitType:
					i = HaveUnitTypeByType(player, temp->Kind.UnitType);
					if (temp->Count ? i < temp->Count : i) {
						goto try_or;
					}
					break;
				case DependRuleUpgrade:
					i = UpgradeIdAllowed(player, temp->Kind.Upgrade - Upgrades) != 'R';
					if (temp->Count ? i : !i) {
						goto try_or;
					}
					break;
			}
			temp = temp->Rule;
		}
		return 1;								// all rules matches.

try_or:
		node = node->Next;
	}

	return 0;										// no rule matches
}

/**
**		Initialize unit and upgrade dependencies.
*/
global void InitDependencies(void)
{
}

/**
**		Save state of the dependencies to file.
**
**		@param file		Output file.
*/
global void SaveDependencies(CLFile* file)
{
	unsigned u;
	const DependRule* node;
	const DependRule* rule;
	const DependRule* temp;

	CLprintf(file,"\n;;; -----------------------------------------\n");
	CLprintf(file,";;; MODULE: dependencies $Id$\n\n");

	// Save all dependencies

	for (u = 0; u < sizeof(DependHash) / sizeof(*DependHash); ++u) {
		node = DependHash[u];
		while (node) {						// all hash links
			CLprintf(file, "(define-dependency '");
			switch (node->Type) {
				case DependRuleUnitType:
					CLprintf(file, "%s", node->Kind.UnitType->Ident);
					break;
				case DependRuleUpgrade:
					CLprintf(file, "%s", node->Kind.Upgrade->Ident);
					break;
			}
			// All or cases

			CLprintf(file, "\n  '(");
			rule = node->Rule;
			for (;;) {
				temp = rule;
				while (temp) {
					switch (temp->Type) {
						case DependRuleUnitType:
							CLprintf(file, "%s", temp->Kind.UnitType->Ident);
							break;
						case DependRuleUpgrade:
							CLprintf(file, "%s", temp->Kind.Upgrade->Ident);
							break;
					}
					temp = temp->Rule;
					if (temp) {
						CLprintf(file," ");
					}
				}
				CLprintf(file, ")");
				if (!(rule = rule->Next)) {
					break;
				}
				CLprintf(file, "\n  'or '( ");
			}

			CLprintf(file, ")\n");

			node = node->Next;
		}
	}
}

/**
**		Clean up unit and upgrade dependencies.
*/
global void CleanDependencies(void)
{
	unsigned u;
	DependRule* node;
	DependRule* rule;
	DependRule* temp;
	DependRule* next;

	// Free all dependencies

	for (u = 0; u < sizeof(DependHash) / sizeof(*DependHash); ++u) {
		node = DependHash[u];
		while (node) {						// all hash links
			// All or cases

			rule = node->Rule;
			while (rule) {
				if (rule) {
					temp = rule->Rule;
					while (temp) {
						next = temp;
						temp = temp->Rule;
						free(next);
					}
				}
				temp = rule;
				rule = rule->Next;
				free(temp);
			}
			temp = node;
			node = node->Next;
			free(temp);
		}
		DependHash[u] = NULL;
	}
}

/*----------------------------------------------------------------------------
--		Ccl part of dependencies
----------------------------------------------------------------------------*/

/**
**		Define a new dependency.
**
**		@param list		List of the dependency.
*/
#if defined(USE_GUILE) || defined(USE_SIOD)
local SCM CclDefineDependency(SCM list)
{
	char* target;
	char* required;
	int count;
	SCM value;
	SCM temp;
	int or_flag;

	value = gh_car(list);
	list = gh_cdr(list);
	target = gh_scm2newstr(value, NULL);

	//
	//		All or rules.
	//
	or_flag = 0;
	while (!gh_null_p(list)) {
		temp = gh_car(list);
		list = gh_cdr(list);

		while (!gh_null_p(temp)) {
			value = gh_car(temp);
			temp = gh_cdr(temp);
			required = gh_scm2newstr(value, NULL);
			count = 1;
			if (!gh_null_p(temp) && gh_exact_p(temp)) {
				value = gh_car(temp);
				count = gh_scm2int(value);
				temp = gh_cdr(temp);
			}

			AddDependency(target, required, count, or_flag);
			free(required);
			or_flag = 0;
		}
		if (!gh_null_p(list)) {
			if (!gh_eq_p(gh_car(list), gh_symbol2scm("or"))) {
				errl("not or symbol", gh_car(list));
				return SCM_UNSPECIFIED;
			}
			or_flag = 1;
			list = gh_cdr(list);
		}
	}
	free(target);

	return SCM_UNSPECIFIED;
}
#elif defined(USE_LUA)
local int CclDefineDependency(lua_State* l)
{
	const char* target;
	const char* required;
	int count;
	const char* value;
	int or_flag;
	int args;
	int j;
	int subargs;
	int k;

	args = lua_gettop(l);
	j = 0;

	target = LuaToString(l, j + 1);
	++j;

	//
	//		All or rules.
	//
	or_flag = 0;
	for (; j < args; ++j) {
		if (!lua_istable(l, j + 1)) {
			lua_pushstring(l, "incorrect argument");
			lua_error(l);
		}
		subargs = luaL_getn(l, j + 1);

		for (k = 0; k < subargs; ++k) {
			lua_rawgeti(l, j + 1, k + 1);
			required = LuaToString(l, -1);
			lua_pop(l, 1);
			count = 1;
			if (k + 1 < subargs) {
				lua_rawgeti(l, j + 1, k + 2);
				if (lua_isnumber(l, -1)) {
					count = LuaToNumber(l, -1);
					++k;
				}
				lua_pop(l, 1);
			}

			AddDependency(target, required, count, or_flag);
			or_flag = 0;
		}
		if (j + 1 < args) {
			++j;
			value = LuaToString(l, j + 1);
			if (strcmp(value, "or")) {
				lua_pushfstring(l, "not or symbol: %s", value);
				lua_error(l);
				return 0;
			}
			or_flag = 1;
		}
	}

	return 0;
}
#endif

/**
**		Get the dependency.
**
**		@todo not written.
**
**		@param target		Unit type or upgrade.
*/
#if defined(USE_GUILE) || defined(USE_SIOD)
local SCM CclGetDependency(SCM target __attribute__((unused)))
{
	DebugLevel0Fn("FIXME: write this %p\n" _C_ target);

	return SCM_UNSPECIFIED;
}
#elif defined(USE_LUA)
local int CclGetDependency(lua_State* l __attribute__((unused)))
{
	DebugLevel0Fn("FIXME: write this %p\n" _C_ l);

	return 0;
}
#endif

/**
**		Check the dependency.
**
**		@todo not written.
**
**		@param target		Unit type or upgrade.
*/
#if defined(USE_GUILE) || defined(USE_SIOD)
local SCM CclCheckDependency(SCM target __attribute__((unused)))
{
	DebugLevel0Fn("FIXME: write this %p\n" _C_ target);

	return SCM_UNSPECIFIED;
}
#elif defined(USE_LUA)
local int CclCheckDependency(lua_State* l __attribute__((unused)))
{
	DebugLevel0Fn("FIXME: write this %p\n" _C_ l);

	return 0;
}
#endif

/**
**		Register CCL features for dependencies.
*/
global void DependenciesCclRegister(void)
{
#if defined(USE_GUILE) || defined(USE_SIOD)
	gh_new_procedureN("define-dependency", CclDefineDependency);
	gh_new_procedure1_0("get-dependency", CclGetDependency);
	gh_new_procedure1_0("check-dependency", CclCheckDependency);
#elif defined(USE_LUA)
	lua_register(Lua, "DefineDependency", CclDefineDependency);
	lua_register(Lua, "GetDependency", CclGetDependency);
	lua_register(Lua, "CheckDependency", CclCheckDependency);
#endif
}

//@}
