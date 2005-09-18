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
/**@name spells.cpp - The spell cast action. */
//
//      (c) Copyright 1998-2005 by Vladi Belperchinov-Shabanski, Lutz Sammer,
//                                 Jimmy Salmon and Joris DAUPHIN
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
// $Id$

/*
** And when we cast our final spell
** And we meet in our dreams
** A place that no one else can go
** Don't ever let your love die
** Don't ever go breaking this spell
*/

//@{

/*----------------------------------------------------------------------------
-- Notes
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
-- Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stratagus.h"

#include "unittype.h"
#include "upgrade.h"
#include "spells.h"
#include "sound.h"
#include "sound_id.h"
#include "missile.h"
#include "map.h"
#include "tileset.h"
#include "ui.h"
#include "actions.h"

/*----------------------------------------------------------------------------
-- Definitons
----------------------------------------------------------------------------*/
#ifndef min
#ifdef __GNUC__
#define min(a,b) ({ typeof(a) _a = a; typeof(b) _b = b; _a < _b ? _a : _b; })
#else
#define min min
static inline min(int a, int b) { return a < b ? a : b; }
#endif
#endif

#ifndef max
#ifdef __GNUC__
#define max(a,b) ({ typeof(a) _a = a; typeof(b) _b = b; _a > _b ? _a : _b; })
#else
#define max max
static inline max(int a, int b) { return a > b ? a : b; }
#endif
#endif

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

/**
** Define the names and effects of all im play available spells.
*/
std::vector<SpellType*> SpellTypeTable;


/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

// ****************************************************************************
// Cast the Spell
// ****************************************************************************

/**
**  Cast demolish
**  @param caster       Unit that casts the spell
**  @param spell        Spell-type pointer
**  @param target       Target unit that spell is addressed to
**  @param x            X coord of target spot when/if target does not exist
**  @param y            Y coord of target spot when/if target does not exist
**
**  @return             =!0 if spell should be repeated, 0 if not
*/
int Demolish::Cast(Unit* caster, const SpellType* spell,
	Unit* target, int x, int y)
{
	int xmin;
	int ymin;
	int xmax;
	int ymax;
	int i;
	int ix;
	int iy;
	int n;
	Unit* table[UnitMax];

	//
	// Allow error margins. (Lame, I know)
	//
	xmin = x - this->Range - 2;
	ymin = y - this->Range - 2;
	xmax = x + this->Range + 2;
	ymax = y + this->Range + 2;
	if (xmin < 0) {
		xmin = 0;
	}
	if (xmax > TheMap.Info.MapWidth - 1) {
		xmax = TheMap.Info.MapWidth - 1;
	}
	if (ymin < 0) {
		ymin = 0;
	}
	if (ymax > TheMap.Info.MapHeight - 1) {
		ymax = TheMap.Info.MapHeight - 1;
	}

	//
	//  Effect of the explosion on units. Don't bother if damage is 0
	//
	if (this->Damage) {
		n = UnitCacheSelect(xmin, ymin, xmax + 1, ymax + 1, table);
		for (i = 0; i < n; ++i) {
			if (table[i]->Type->UnitType != UnitTypeFly && table[i]->Orders[0].Action != UnitActionDie &&
					MapDistanceToUnit(x, y, table[i]) <= this->Range) {
				// Don't hit flying units!
				HitUnit(caster, table[i], this->Damage);
			}
		}
	}

	//
	// Terrain effect of the explosion
	//
	for (ix = xmin; ix <= xmax; ++ix) {
		for (iy = ymin; iy <= ymax; ++iy) {
			n = TheMap.Fields[ix + iy * TheMap.Info.MapWidth].Flags;
			if (MapDistance(ix, iy, x, y ) > this->Range) {
				// Not in circle range
				continue;
			} else if (n & MapFieldWall) {
				MapRemoveWall(ix, iy);
			} else if (n & MapFieldRocks) {
				MapClearTile(MapFieldRocks, ix, iy);
			} else if (n & MapFieldForest) {
				MapClearTile(MapFieldForest, ix, iy);
			}
		}
	}
	return 1;
}

/**
** Cast circle of power.
**
**  @param caster       Unit that casts the spell
**  @param spell        Spell-type pointer
**  @param target       Target unit that spell is addressed to
**  @param x            X coord of target spot when/if target does not exist
**  @param y            Y coord of target spot when/if target does not exist
**
**  @return             =!0 if spell should be repeated, 0 if not
*/
int SpawnPortal::Cast(Unit* caster, const SpellType* spell,
	Unit* target, int x, int y)
{
	// FIXME: vladi: cop should be placed only on explored land
	Unit* portal;
	UnitType* ptype;

	ptype = this->PortalType;

	DebugPrint("Spawning a portal exit.\n");
	portal = caster->Goal;
	if (portal) {
		MoveUnitToXY(portal, x, y);
	} else {
		portal = MakeUnitAndPlace(x, y, ptype, &Players[PlayerNumNeutral]);
	}
	//  Goal is used to link to destination circle of power
	caster->Goal = portal;
	RefsIncrease(portal);
	//FIXME: setting destination circle of power should use mana
	return 0;
}

/**
** Cast Area Adjust Vitals on all valid units in range.
**
**  @param caster       Unit that casts the spell
**  @param spell        Spell-type pointer
**  @param target       Target unit that spell is addressed to
**  @param x            X coord of target spot when/if target does not exist
**  @param y            Y coord of target spot when/if target does not exist
**
**  @return             =!0 if spell should be repeated, 0 if not
*/
int AreaAdjustVitals::Cast(Unit* caster, const SpellType* spell,
	Unit* target, int x, int y)
{
	Unit* units[UnitMax];
	int nunits;
	int j;
	int hp;
	int mana;

	// Get all the units around the unit
	nunits = UnitCacheSelect(x - spell->Range,
		y - spell->Range,
		x + spell->Range + caster->Type->Width,
		y + spell->Range + caster->Type->Height,
		units);
	hp = this->HP;
	mana = this->Mana;
	caster->Variable[MANA_INDEX].Value -= spell->ManaCost;
	for (j = 0; j < nunits; ++j) {
		target = units[j];
// if (!PassCondition(caster, spell, target, x, y) {
		if (!CanCastSpell(caster, spell, target, x, y)) {
			continue;
		}
		if (hp < 0) {
			HitUnit(caster, target, -hp);
		} else {
			target->Variable[HP_INDEX].Value += hp;
			if (target->Variable[HP_INDEX].Value > target->Variable[HP_INDEX].Max) {
				target->Variable[HP_INDEX].Value = target->Variable[HP_INDEX].Max;
			}
		}
		target->Variable[MANA_INDEX].Value += mana;
		if (target->Variable[MANA_INDEX].Value < 0) {
			target->Variable[MANA_INDEX].Value = 0;
		}
		if (target->Variable[MANA_INDEX].Value > target->Variable[MANA_INDEX].Max) {
			target->Variable[MANA_INDEX].Value = target->Variable[MANA_INDEX].Max;
		}
	}
	return 0;
}

/**
** Cast area bombardment.
**
**  @param caster       Unit that casts the spell
**  @param spell        Spell-type pointer
**  @param target       Target unit that spell is addressed to
**  @param x            X coord of target spot when/if target does not exist
**  @param y            Y coord of target spot when/if target does not exist
**
**  @return             =!0 if spell should be repeated, 0 if not
**  @internal: vladi: blizzard differs than original in this way:
**   original: launches 50 shards at 5 random spots x 10 for 25 mana.
*/
int AreaBombardment::Cast(Unit* caster, const SpellType* spell,
	Unit* target, int x, int y)
{
	int fields;
	int shards;
	int damage;
	struct _missile_* mis;
	int offsetx;
	int offsety;
	int dx;
	int dy;
	int i;
	MissileType *missile;

	mis = NULL;

	fields = this->Fields;
	shards = this->Shards;
	damage = this->Damage;
	offsetx = this->StartOffsetX;
	offsety = this->StartOffsetY;
	missile = this->Missile;
	while (fields--) {
			// FIXME: radius configurable...
		do {
			// find new destination in the map
			dx = x + SyncRand() % 5 - 2;
			dy = y + SyncRand() % 5 - 2;
		} while (dx < 0 && dy < 0 && dx >= TheMap.Info.MapWidth && dy >= TheMap.Info.MapHeight);
		for (i = 0; i < shards; ++i) {
			mis = MakeMissile(missile,
				dx * TileSizeX + TileSizeX / 2 + offsetx,
				dy * TileSizeY + TileSizeY / 2 + offsety,
				dx * TileSizeX + TileSizeX / 2,
				dy * TileSizeY + TileSizeY / 2);
			//  FIXME: This is just patched up, it works, but I have no idea why.
			//  FIXME: What is the reasoning behind all this?
			if (mis->Type->Speed) {
				mis->Delay = i * mis->Type->Sleep * 2 * TileSizeX / mis->Type->Speed;
			} else {
				mis->Delay = i * mis->Type->Sleep * mis->Type->G->NumFrames;
			}
			mis->Damage = damage;
			// FIXME: not correct -- blizzard should continue even if mage is
			//    destroyed (though it will be quite short time...)
			mis->SourceUnit = caster;
			RefsIncrease(caster);
		}
	}
	return 1;
}

/**
** Evaluate missile location description.
**
** @param location     Parameters for location.
** @param caster       Unit that casts the spell
** @param target       Target unit that spell is addressed to
** @param x            X coord of target spot when/if target does not exist
** @param y            Y coord of target spot when/if target does not exist
** @param resx         pointer to X coord of the result
** @param resy         pointer to Y coord of the result
*/
static void EvaluateMissileLocation(const SpellActionMissileLocation* location,
	Unit* caster, Unit* target, int x, int y, int* resx, int* resy)
{
	if (location->Base == LocBaseCaster) {
		*resx = caster->X * TileSizeX + TileSizeX / 2;
		*resy = caster->Y * TileSizeY + TileSizeY / 2;
	} else {
		if (target) {
			*resx = target->X * TileSizeX + TileSizeX / 2;
			*resy = target->Y * TileSizeY + TileSizeY / 2;
		} else {
			*resx = x * TileSizeX + TileSizeX / 2;
			*resy = y * TileSizeY + TileSizeY / 2;
		}
	}
	*resx += location->AddX;
	if (location->AddRandX) {
		*resx += SyncRand() % location->AddRandX;
	}
	*resy += location->AddY;
	if (location->AddRandY) {
		*resy += SyncRand() % location->AddRandY;
	}
}

/**
** Cast spawn missile.
**
**  @param caster       Unit that casts the spell
**  @param spell        Spell-type pointer
**  @param target       Target unit that spell is addressed to
**  @param x            X coord of target spot when/if target does not exist
**  @param y            Y coord of target spot when/if target does not exist
**
**  @return             =!0 if spell should be repeated, 0 if not
*/
int SpawnMissile::Cast(Unit* caster, const SpellType* spell,
	Unit* target, int x, int y)
{
	struct _missile_* missile;
	int sx;
	int sy;
	int dx;
	int dy;

	EvaluateMissileLocation(&this->StartPoint,
		caster, target, x, y, &sx, &sy);
	EvaluateMissileLocation(&this->EndPoint,
		caster, target, x, y, &dx, &dy);

	missile = MakeMissile(this->Missile, sx, sy, dx, dy);
	missile->TTL = this->TTL;
	missile->Delay = this->Delay;
	missile->Damage = this->Damage;
	if (missile->Damage != 0) {
		missile->SourceUnit = caster;
		RefsIncrease(caster);
	}
	if ((missile->TargetUnit = target)) {
		RefsIncrease(target);
	}
	return 1;
}

/**
**  Adjust User Variables.
**
**  @param caster  Unit that casts the spell
**  @param spell   Spell-type pointer
**  @param target  Target
**  @param x       X coord of target spot when/if target does not exist
**  @param y       Y coord of target spot when/if target does not exist
**
**  @return        =!0 if spell should be repeated, 0 if not
*/
int AdjustVariable::Cast(Unit* caster, const SpellType* spell,
	Unit* target, int x, int y)
{
	int i;
	Unit* unit;    // unit to modify.

	for (i = 0; i < UnitTypeVar.NumberVariable; ++i) {
		unit = (this->Var[i].TargetIsCaster) ? caster : target;
		if (!unit) {
			continue;
		}
		// Enable flag.
		if (this->Var[i].ModifEnable) {
			unit->Variable[i].Enable = this->Var[i].Enable;
		}
		unit->Variable[i].Enable ^= this->Var[i].InvertEnable;

		// Max field
		if (this->Var[i].ModifMax) {
			unit->Variable[i].Max = this->Var[i].Max;
		}
		unit->Variable[i].Max += this->Var[i].AddMax;

		// Increase field
		if (this->Var[i].ModifIncrease) {
			unit->Variable[i].Increase = this->Var[i].Increase;
		}
		unit->Variable[i].Increase += this->Var[i].AddIncrease;

		// Value field
		if (this->Var[i].ModifValue) {
			unit->Variable[i].Value = this->Var[i].Value;
		}
		unit->Variable[i].Value += this->Var[i].AddValue;
		unit->Variable[i].Value += this->Var[i].IncreaseTime
			* unit->Variable[i].Increase;

		if (unit->Variable[i].Value <= 0) {
			unit->Variable[i].Value = 0;
		} else if (unit->Variable[i].Value > unit->Variable[i].Max) {
			unit->Variable[i].Value = unit->Variable[i].Max;
		}
	}
	return 0;
}


/**
** Cast healing. (or exorcism)
**
**  @param caster       Unit that casts the spell
**  @param spell        Spell-type pointer
**  @param target       Target unit that spell is addressed to
**  @param x            X coord of target spot when/if target does not exist
**  @param y            Y coord of target spot when/if target does not exist
**
**  @return             =!0 if spell should be repeated, 0 if not
*/
int AdjustVitals::Cast(Unit* caster, const SpellType* spell,
	Unit* target, int x, int y)
{
	int castcount;
	int diffHP;
	int diffMana;
	int hp;
	int mana;
	int manacost;

	Assert(caster);
	Assert(spell);
	if (!target) {
		return 0;
	}

	hp = this->HP;
	mana = this->Mana;
	manacost = spell->ManaCost;

	//  Healing and harming
	if (hp > 0) {
		diffHP = target->Variable[HP_INDEX].Max - target->Variable[HP_INDEX].Value;
	} else {
		diffHP = target->Variable[HP_INDEX].Value;
	}
	if (mana > 0) {
		diffMana = target->Stats->Variables[MANA_INDEX].Max - target->Variable[MANA_INDEX].Value;
	} else {
		diffMana = target->Variable[MANA_INDEX].Value;
	}

	//  When harming cast again to send the hp to negative values.
	//  Carefull, a perfect 0 target hp kills too.
	//  Avoid div by 0 errors too!
	castcount = 0;
	if (hp) {
		castcount = max(castcount, diffHP / abs(hp) + (((hp < 0) &&
			(diffHP % (-hp) > 0)) ? 1 : 0));
	}
	if (mana) {
		castcount = max(castcount, diffMana / abs(mana) + (((mana < 0) &&
			(diffMana % (-mana) > 0)) ? 1 : 0));
	}
	if (manacost) {
		castcount = min(castcount, caster->Variable[MANA_INDEX].Value / manacost);
	}
	if (this->MaxMultiCast) {
		castcount = min(castcount, this->MaxMultiCast);
	}

	caster->Variable[MANA_INDEX].Value -= castcount * manacost;
	if (hp < 0) {
		HitUnit(caster, target, -(castcount * hp));
	} else {
		target->Variable[HP_INDEX].Value += castcount * hp;
		if (target->Variable[HP_INDEX].Value > target->Variable[HP_INDEX].Max) {
			target->Variable[HP_INDEX].Value = target->Variable[HP_INDEX].Max;
		}
	}
	target->Variable[MANA_INDEX].Value += castcount * mana;
	if (target->Variable[MANA_INDEX].Value < 0) {
		target->Variable[MANA_INDEX].Value = 0;
	}
	if (target->Variable[MANA_INDEX].Value > target->Variable[MANA_INDEX].Max) {
		target->Variable[MANA_INDEX].Value = target->Variable[MANA_INDEX].Max;
	}

	return 0;
}

/**
** Cast polymorph.
**
**  @param caster       Unit that casts the spell
**  @param spell        Spell-type pointer
**  @param target       Target unit that spell is addressed to
**  @param x            X coord of target spot when/if target does not exist
**  @param y            Y coord of target spot when/if target does not exist
**
**  @return             =!0 if spell should be repeated, 0 if not
*/
int Polymorph::Cast(Unit* caster, const SpellType* spell,
	Unit* target, int x, int y)
{
	int i;
	int j;
	UnitType* type;

	if (!target) {
		return 0;
	}

	type = this->NewForm;

	x = x - type->TileWidth / 2;
	y = y - type->TileHeight / 2;

	caster->Player->Score += target->Type->Points;
	if (IsEnemy(caster->Player, target)) {
		if (target->Type->Building) {
			caster->Player->TotalRazings++;
		} else {
			caster->Player->TotalKills++;
		}
		if (UseHPForXp) {
			caster->Variable[XP_INDEX].Max += target->Variable[HP_INDEX].Value;
		} else {
			caster->Variable[XP_INDEX].Max += target->Type->Points;
		}
		caster->Variable[XP_INDEX].Value = caster->Variable[XP_INDEX].Max;
		caster->Variable[KILL_INDEX].Value++;
		caster->Variable[KILL_INDEX].Max++;
		caster->Variable[KILL_INDEX].Enable = 1;
	}

	// as said somewhere else -- no corpses :)
	RemoveUnit(target, NULL);
	for (i = 0; i < type->TileWidth; ++i) {
		for (j = 0; j < type->TileHeight; ++j) {
			if (!UnitTypeCanBeAt(type, x + i, y + j)) {
				PlaceUnit(target, target->X, target->Y);
				return 0;
			}
		}
	}
	caster->Variable[MANA_INDEX].Value -= spell->ManaCost;
	if (this->PlayerNeutral) {
		MakeUnitAndPlace(x, y, type, Players + PlayerNumNeutral);
	} else {
		MakeUnitAndPlace(x, y, type, target->Player);
	}
	UnitLost(target);
	UnitClearOrders(target);
	ReleaseUnit(target);
	return 1;
}

/**
** Cast capture.
**
**  @param caster       Unit that casts the spell
**  @param spell        Spell-type pointer
**  @param target       Target unit that spell is addressed to
**  @param x            X coord of target spot when/if target does not exist
**  @param y            Y coord of target spot when/if target does not exist
**
**  @return             =!0 if spell should be repeated, 0 if not
*/
int Capture::Cast(Unit* caster, const SpellType* spell,
	Unit* target, int x, int y)
{
	if (!target || caster->Player == target->Player) {
		return 0;
	}

	if (this->DamagePercent) {
		if ((100 * target->Variable[HP_INDEX].Value) /
			target->Variable[HP_INDEX].Max > this->DamagePercent &&
			target->Variable[HP_INDEX].Value > this->Damage) {
				HitUnit(caster, target, this->Damage);
				if (this->SacrificeEnable) {
					// No corpse.
					RemoveUnit(caster, NULL);
					UnitLost(caster);
					UnitClearOrders(caster);
				}
				return 1;
			}
		}
	caster->Player->Score += target->Type->Points;
	if (IsEnemy(caster->Player, target)) {
		if (target->Type->Building) {
			caster->Player->TotalRazings++;
		} else {
			caster->Player->TotalKills++;
		}
		if (UseHPForXp) {
			caster->Variable[XP_INDEX].Max += target->Variable[HP_INDEX].Value;
		} else {
			caster->Variable[XP_INDEX].Max += target->Type->Points;
		}
		caster->Variable[XP_INDEX].Value = caster->Variable[XP_INDEX].Max;
		caster->Variable[KILL_INDEX].Value++;
		caster->Variable[KILL_INDEX].Max++;
		caster->Variable[KILL_INDEX].Enable = 1;
	}
	ChangeUnitOwner(target, caster->Player);
	if (this->SacrificeEnable) {
		// No corpse.
		RemoveUnit(caster, NULL);
		UnitLost(caster);
		UnitClearOrders(caster);
	} else {
		caster->Variable[MANA_INDEX].Value -= spell->ManaCost;
	}
	UnitClearOrders(target);
	return 0;
}

/**
** Cast summon spell.
**
**  @param caster       Unit that casts the spell
**  @param spell        Spell-type pointer
**  @param target       Target unit that spell is addressed to
**  @param x            X coord of target spot when/if target does not exist
**  @param y            Y coord of target spot when/if target does not exist
**
**  @return             =!0 if spell should be repeated, 0 if not
*/
int Summon::Cast(Unit* caster, const SpellType* spell,
	Unit* target, int x, int y)
{
	int ttl;
	int cansummon;
	int n;
	Unit* table[UnitMax];
	Unit* unit;
	struct _unit_type_* unittype;

	unittype = this->UnitType;
	ttl = this->TTL;

	if (this->RequireCorpse) {
		n = UnitCacheSelect(x - 1, y - 1, x + 2, y + 2, table);
		cansummon = 0;
		while (n) {
			n--;
			unit = table[n];
			if (unit->Orders[0].Action == UnitActionDie && !unit->Type->Building) {
				//
				//  Found a corpse. eliminate it and proceed to summoning.
				//
				x = unit->X;
				y = unit->Y;
				ReleaseUnit(unit);
				cansummon = 1;
				break;
			}
		}
	} else {
		cansummon = 1;
	}

	if (cansummon) {
		DebugPrint("Summoning a %s\n" _C_ unittype->Name);

		//
		// Create units.
		// FIXME: do summoned units count on food?
		//
		target = MakeUnit(unittype, caster->Player);
		if (target != NoUnitP) {
			// This is a hack to walk around behaviour of DropOutOnSide
			target->X = x + 1;
			target->Y = y;
			DropOutOnSide(target, LookingW, 0, 0); // FIXME : 0,0) : good parameter ?
			//
			//  set life span. ttl=0 results in a permanent unit.
			//
			if (ttl) {
				target->TTL = GameCycle + ttl;
			}

			caster->Variable[MANA_INDEX].Value -= spell->ManaCost;
		} else {
			DebugPrint("Unable to allocate Unit");
		}
		return 1;
	}
	return 0;
}

// ****************************************************************************
// Target constructor
// ****************************************************************************

/**
**  Target constructor.
**
**  @param t            Type of target (unit, position).
**  @param unit         Unit target.
**  @param x            x coord of the target.
**  @param y            y coord of the target.
**  @return             the new target.
*/
static Target* NewTarget(TargetType t, const Unit* unit, int x, int y)
{
	Target* target;

	target = (Target*)malloc(sizeof(*target));

	target->which_sort_of_target = t;
	target->unit = (Unit*)unit;
	target->X = x;
	target->Y = y;
	return target;
}

/**
**  Target constructor for unit.
**
**  @param unit     Target unit.
**
**  @return the new target.
*/
static Target* NewTargetUnit(const Unit* unit)
{
	return NewTarget(TargetUnit, unit, 0, 0);
}

#if 0
/**
**  Target constructor for position.
**
**  @param x        x position.
**  @param y        y position.
**
**  @return the new target.
*/
static Target* NewTargetPosition(int x, int y)
{
	return NewTarget(TargetPosition, NULL, x, y);
}
#endif

// ****************************************************************************
// Main local functions
// ****************************************************************************

/**
**  Check the condition.
**
**  @param caster      Pointer to caster unit.
**  @param spell       Pointer to the spell to cast.
**  @param target      Pointer to target unit, or 0 if it is a position spell.
**  @param x           X position, or -1 if it is an unit spell.
**  @param y           Y position, or -1 if it is an unit spell.
**  @param condition   Pointer to condition info.
**
**  @return            1 if passed, 0 otherwise.
*/
static int PassCondition(const Unit* caster, const SpellType* spell, const Unit* target,
	int x, int y, const ConditionInfo* condition)
{
	int i;

	if (caster->Variable[MANA_INDEX].Value < spell->ManaCost) { // Check caster mana.
		return 0;
	}
	if (spell->Target == TargetUnit) { // Casting an unit spell without a target.
		if ((!target) || target->Destroyed || target->Orders->Action == UnitActionDie) {
			return 0;
		}
	}
	if (!condition) { // no condition, pass.
		return 1;
	}

	for (i = 0; i < UnitTypeVar.NumberVariable; i++) { // for custom variables
		const Unit *unit;

		unit = (condition->Variable[i].ConditionApplyOnCaster) ? caster : target;
		//  Spell should target location and have unit condition.
		if (unit == NULL) {
			continue;
		}
		if (condition->Variable[i].Enable != CONDITION_TRUE) {
			if ((condition->Variable[i].Enable == CONDITION_ONLY) ^ (unit->Variable[i].Enable)) {
				return 0;
			}
		}
	// Value and Max
		if (condition->Variable[i].MinValue >= unit->Variable[i].Value) {
			return 0;
		}
		if (condition->Variable[i].MaxValue != -1 &&
			condition->Variable[i].MaxValue <= unit->Variable[i].Value) {
			return 0;
		}

		if (condition->Variable[i].MinMax >= unit->Variable[i].Max) {
			return 0;
		}

		if (!unit->Variable[i].Max) {
			continue;
		}
	// Percent
		if (condition->Variable[i].MinValuePercent * unit->Variable[i].Max
			>= 100 * unit->Variable[i].Value) {
			return 0;
		}
		if (condition->Variable[i].MaxValuePercent * unit->Variable[i].Max
			<= 100 * unit->Variable[i].Value) {
			return 0;
		}
	}

	if (!target) {
		return 1;
	}
	for (i = 0; i < UnitTypeVar.NumberBoolFlag; i++) { // User defined flags
		if (condition->BoolFlag[i] != CONDITION_TRUE) {
			if ((condition->BoolFlag[i] == CONDITION_ONLY) ^ (target->Type->BoolFlag[i])) {
				return 0;
			}
		}
	}
	if (condition->Alliance != CONDITION_TRUE) {
		if ((condition->Alliance == CONDITION_ONLY) ^
				// own units could be not allied ?
				(IsAllied(caster->Player, target) || target->Player == caster->Player)) {
			return 0;
		}
	}
	if (condition->Opponent != CONDITION_TRUE) {
		if ((condition->Opponent == CONDITION_ONLY) ^
				(IsEnemy(caster->Player, target) && 1)) {
			return 0;
		}
	}

	if (condition->TargetSelf != CONDITION_TRUE) {
		if ((condition->TargetSelf == CONDITION_ONLY) ^ (caster == target)) {
			return 0;
		}
	}
	return 1;
}

/**
**  Select the target for the autocast.
**
**  @param caster    Unit who would cast the spell.
**  @param spell     Spell-type pointer.
**
**  @return          Target* choosen target or Null if spell can't be cast.
**  @todo FIXME: should be global (for AI) ???
**  @todo FIXME: write for position target.
*/
static Target* SelectTargetUnitsOfAutoCast(const Unit* caster, const SpellType* spell)
{
	Unit* table[UnitMax];
	int x;
	int y;
	int range;
	int nunits;
	int i;
	int j;
	int combat;
	AutoCastInfo* autocast;

	// Ai cast should be a lot better. Use autocast if not found.
	if (caster->Player->AiEnabled && spell->AICast) {
		autocast = spell->AICast;
	} else {
		autocast = spell->AutoCast;
	}
	Assert(autocast);
	x = caster->X;
	y = caster->Y;
	range = spell->AutoCast->Range;

	//
	// Select all units aroung the caster
	//
	nunits = UnitCacheSelect(caster->X - range, caster->Y - range,
		caster->X + range + caster->Type->TileWidth,
		caster->Y + range + caster->Type->TileHeight, table);
	//
	//  Check every unit if it is hostile
	//
	combat = 0;
	for (i = 0; i < nunits; ++i) {
		if (IsEnemy(caster->Player, table[i]) && !table[i]->Type->Coward) {
			combat = 1;
		}
	}

	//
	// Check generic conditions. FIXME: a better way to do this?
	//
	if (autocast->Combat != CONDITION_TRUE) {
		if ((autocast->Combat == CONDITION_ONLY) ^ (combat)) {
			return 0;
		}
	}

	switch (spell->Target) {
		case TargetSelf :
			if (PassCondition(caster, spell, caster, x, y, spell->Condition) &&
					PassCondition(caster, spell, caster, x, y, autocast->Condition)) {
					return NewTargetUnit(caster);
				}
			return 0;
		case TargetPosition:
			return 0;
			//  Autocast with a position? That's hard
			//  Possibilities: cast reveal-map on a dark region
			//  Cast raise dead on a bunch of corpses. That would rule.
			//  Cast summon until out of mana in the heat of battle. Trivial?
			//  Find a tight group of units and cast area-damage spells. HARD,
			//  but it is a must-have for AI. What about area-heal?
		case TargetUnit:
			//
			// The units are already selected.
			//  Check every unit if it is a possible target
			//
			for (i = 0, j = 0; i < nunits; ++i) {
				//  FIXME: autocast conditions should include normal conditions.
				//  FIXME: no, really, they should.
				if (PassCondition(caster, spell, table[i], x, y, spell->Condition) &&
						PassCondition(caster, spell, table[i], x, y, autocast->Condition)) {
					table[j++] = table[i];
				}
			}
			nunits = j;
			//
			// Now select the best unit to target.
			// FIXME: Some really smart way to do this.
			// FIXME: Heal the unit with the lowest hit-points
			// FIXME: Bloodlust the unit with the highest hit-point
			// FIMXE: it will survive more
			//
			if (nunits != 0) {
#if 0
				// For the best target???
				sort(table, nb_units, spell->autocast->f_order);
				return NewTargetUnit(table[0]);
#else
				// Best unit, random unit, oh well, same stuff.
				i = SyncRand() % nunits;
				return NewTargetUnit(table[i]);
#endif
			}
			break;
		default:
			// Something is wrong
			DebugPrint("Spell is screwed up, unknown target type\n");
			Assert(0);
			return NULL;
			break;
		}
	return NULL; // Can't spell the auto-cast.
}

// ****************************************************************************
// Public spell functions
// ****************************************************************************

// ****************************************************************************
// Constructor and destructor
// ****************************************************************************

/**
** Spells constructor, inits spell id's and sounds
*/
void InitSpells(void)
{
}

/**
**  Get spell-type struct pointer by string identifier.
**
**  @param ident  Spell identifier.
**
**  @return       spell-type struct pointer.
*/
SpellType* SpellTypeByIdent(const char* ident)
{
	for (std::vector<SpellType *>::iterator i = SpellTypeTable.begin(); i < SpellTypeTable.end(); ++i) {
		if (strcmp((*i)->Ident, ident) == 0) {
			return *i;
		}
	}
	return NULL;
}

// ****************************************************************************
// CanAutoCastSpell, CanCastSpell, AutoCastSpell, CastSpell.
// ****************************************************************************

/**
**  Check if spell is research for player \p player.
**  @param player    player for who we want to know if he knows the spell.
**  @param spellid   id of the spell to check.
**
**  @return          0 if spell is not available, else no null.
*/
int SpellIsAvailable(const Player* player, int spellid)
{
	int dependencyId;

	dependencyId = SpellTypeTable[spellid]->DependencyId;

	return dependencyId == -1 || UpgradeIdAllowed(player, dependencyId) == 'R';
}

/**
**  Check if unit can cast the spell.
**
**  @param caster    Unit that casts the spell
**  @param spell     Spell-type pointer
**  @param target    Target unit that spell is addressed to
**  @param x         X coord of target spot when/if target does not exist
**  @param y         Y coord of target spot when/if target does not exist
**
**  @return          =!0 if spell should/can casted, 0 if not
**  @note caster must know the spell, and spell must be researched.
*/
int CanCastSpell(const Unit* caster, const SpellType* spell,
	const Unit* target, int x, int y)
{
	if (spell->Target == TargetUnit && target == NULL) {
		return 0;
	}
	return PassCondition(caster, spell, target, x, y, spell->Condition);
}

/**
**  Check if the spell can be auto cast and cast it.
**
**  @param caster    Unit who can cast the spell.
**  @param spell     Spell-type pointer.
**
**  @return          1 if spell is casted, 0 if not.
*/
int AutoCastSpell(Unit* caster, const SpellType* spell)
{
	Target* target;

	//  Check for mana, trivial optimization.
	if (!SpellIsAvailable(caster->Player, spell->Slot)
		|| caster->Variable[MANA_INDEX].Value < spell->ManaCost) {
		return 0;
	}
	target = SelectTargetUnitsOfAutoCast(caster, spell);
	if (target == NULL) {
		return 0;
	} else {
		// Must move before ?
		// FIXME: SpellType* of CommandSpellCast must be const.
		CommandSpellCast(caster, target->X, target->Y, target->unit,
			(SpellType*)spell, FlushCommands);
		free(target);
	}
	return 1;
}

/**
** Spell cast!
**
** @param caster    Unit that casts the spell
** @param spell     Spell-type pointer
** @param target    Target unit that spell is addressed to
** @param x         X coord of target spot when/if target does not exist
** @param y         Y coord of target spot when/if target does not exist
**
** @return          !=0 if spell should/can continue or 0 to stop
*/
int SpellCast(Unit* caster, const SpellType* spell, Unit* target,
	int x, int y)
{
	int cont;             // Should we recast the spell.
	int mustSubtractMana; // false if action which have their own calculation is present.

	caster->Variable[INVISIBLE_INDEX].Value = 0;// unit is invisible until attacks // FIXME: Must be configurable
	if (target) {
		x = target->X;
		y = target->Y;
	}
	//
	// For TargetSelf, you target.... YOURSELF
	//
	if (spell->Target == TargetSelf) {
		x = caster->X;
		y = caster->Y;
		target = caster;
	}
	DebugPrint("Spell cast: (%s), %s -> %s (%d,%d)\n" _C_ spell->Ident _C_
		caster->Type->Name _C_ target ? target->Type->Name : "none" _C_ x _C_ y);
	if (CanCastSpell(caster, spell, target, x, y)) {
		cont = 1;
		mustSubtractMana = 1;
		//
		//  Ugly hack, CastAdjustVitals makes it's own mana calculation.
		//
		PlayGameSound(spell->SoundWhenCast.Sound, MaxSampleVolume);
		for (std::vector<SpellActionType*>::const_iterator act = spell->Action.begin();
			act != spell->Action.end();	++act) {
			if ((*act)->ModifyManaCaster) {
				mustSubtractMana = 0;
			}
			cont = cont & (*act)->Cast(caster, spell, target, x, y);
		}
		if (mustSubtractMana) {
			caster->Variable[MANA_INDEX].Value -= spell->ManaCost;
		}
		//
		// Spells like blizzard are casted again.
		// This is sort of confusing, we do the test again, to
		// check if it will be possible to cast again. Otherwise,
		// when you're out of mana the caster will try again ( do the
		// anim but fail in this proc.
		//
		if (spell->RepeatCast && cont) {
			return CanCastSpell(caster, spell, target, x, y);
		}
	}
	//
	// Can't cast, STOP.
	//
	return 0;
}


/**
**  SpellType constructor.
*/
SpellType::SpellType(int slot, const char *identname) :
	Name(NULL), Slot(slot),
	Target(), Action(),
	Range(0), ManaCost(0), RepeatCast(0),
	DependencyId(-1), Condition(NULL),
	AutoCast(NULL), AICast(NULL)
{
	Ident = strdup(identname);
	SoundWhenCast.Name = NULL;
	SoundWhenCast.Sound = NULL;
}

/**
**  SpellType destructor.
*/
SpellType::~SpellType()
{
	free(Ident);
	free(Name);
	for (std::vector<SpellActionType *>::iterator act = Action.begin(); act != Action.end(); ++act) {
		delete *act;
	}
	Action.clear();

	delete Condition;
	//
	// Free Autocast.
	//
	delete AutoCast;
	delete AICast;

	free(SoundWhenCast.Name);
}


/**
** Cleanup the spell subsystem.
*/
void CleanSpells(void)
{
	DebugPrint("Cleaning spells.\n");
	for (std::vector<SpellType *>::iterator i = SpellTypeTable.begin(); i < SpellTypeTable.end(); ++i) {
		delete *i;
	}
	SpellTypeTable.clear();
}

//@}
