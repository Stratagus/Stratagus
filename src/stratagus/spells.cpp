//       _________ __                 __                               
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/ 
//  ______________________                           ______________________
//			  T H E	  W A R	  B E G I N S
//	   Stratagus - A free fantasy real time strategy game engine
//
/**@name spells.c	-	The spell cast action. */
//
//	(c) Copyright 1998-2003 by Vladi Belperchinov-Shabanski, Lutz Sammer,
//	                           Jimmy Salmon and Joris DAUPHIN
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

/*
**	And when we cast our final spell
**	And we meet in our dreams
**	A place that no one else can go
**	Don't ever let your love die
**	Don't ever go breaking this spell
*/

//@{

/*----------------------------------------------------------------------------
--	Notes
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stratagus.h"

#include "spells.h"
#include "sound.h"
#include "sound_id.h"
#include "missile.h"
#include "map.h"
#include "ui.h"
#include "actions.h"

/*----------------------------------------------------------------------------
--	Definitons
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

/**
**	Define the names and effects of all im play available spells.
*/
global SpellType* SpellTypeTable;


/// How many spell-types are available
global int SpellTypeCount;

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

// ****************************************************************************
// Cast the Spell
// ****************************************************************************

/**
**	Cast circle of power.
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		=!0 if spell should be repeated, 0 if not
*/
global int CastSpawnPortal(Unit* caster, const SpellType* spell __attribute__((unused)),
    Unit* target __attribute__((unused)), int x, int y)
{
    // FIXME: vladi: cop should be placed only on explored land
    Unit* portal;
    UnitType* ptype;
    
    DebugCheck(!caster);
    DebugCheck(!spell);
    DebugCheck(!spell->Action);
    DebugCheck(!spell->Action->Data.SpawnPortal.PortalType);

    ptype = spell->Action->Data.SpawnPortal.PortalType;

    portal = caster->Goal;
    if (portal) {
	// FIXME: if cop is already defined --> move it, but it doesn't work?
	RemoveUnit(portal, NULL);
	PlaceUnit(portal, x, y);
    } else {
	portal = MakeUnitAndPlace(x, y, ptype, &Players[PlayerMax - 1]);
    }
    MakeMissile(spell->Missile,
	x * TileSizeX + TileSizeX / 2, y * TileSizeY + TileSizeY / 2,
	x * TileSizeX + TileSizeX / 2, y * TileSizeY + TileSizeY / 2);
    //  Goal is used to link to destination circle of power
    caster->Goal = portal;
    RefsDebugCheck(!portal->Refs || portal->Destroyed);
    portal->Refs++;
    //FIXME: setting destination circle of power should use mana
    return 0;
}

//	AreaBombardment
//   NOTE: vladi: blizzard differs than original in this way:
//   original: launches 50 shards at 5 random spots x 10 for 25 mana.

/**
**	Cast area bombardment.
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		=!0 if spell should be repeated, 0 if not
*/
global int CastAreaBombardment(Unit* caster, const SpellType* spell,
    Unit* target __attribute__((unused)), int x, int y)
{
    int fields;
    int shards;
    int damage;
    Missile* mis;
    int offsetx;
    int offsety;
    int dx;
    int dy;
    int i;

    DebugCheck(!caster);
    DebugCheck(!spell);
    DebugCheck(!spell->Action);
    //assert(x in range, y in range);

    mis = NULL;

    fields = spell->Action->Data.AreaBombardment.Fields;
    shards = spell->Action->Data.AreaBombardment.Shards;
    damage = spell->Action->Data.AreaBombardment.Damage;
    offsetx = spell->Action->Data.AreaBombardment.StartOffsetX;
    offsety = spell->Action->Data.AreaBombardment.StartOffsetY;
    while (fields--) {
    	// FIXME : radius configurable...
	do {
	    // find new destination in the map
	    dx = x + SyncRand() % 5 - 2;
	    dy = y + SyncRand() % 5 - 2;
	} while (dx < 0 && dy < 0 && dx >= TheMap.Width && dy >= TheMap.Height);
	for (i = 0; i < shards; ++i) {
	    mis = MakeMissile(spell->Missile,
		dx * TileSizeX + TileSizeX / 2 + offsetx,
		dy * TileSizeY + TileSizeY / 2 + offsety,
		dx * TileSizeX + TileSizeX / 2,
		dy * TileSizeY + TileSizeY / 2);
	    //  FIXME: This is just patched up, it works, but I have no idea why.
	    //  FIXME: What is the reasoning behind all this?
	    if (mis->Type->Speed) {
		mis->Delay = i * mis->Type->Sleep * 2 * TileSizeX / mis->Type->Speed;
	    } else {
		mis->Delay = i * mis->Type->Sleep * VideoGraphicFrames(mis->Type->Sprite);
	    }
	    mis->Damage = damage;
	    // FIXME: not correct -- blizzard should continue even if mage is
	    //       destroyed (though it will be quite short time...)
	    mis->SourceUnit = caster;
	    RefsDebugCheck(!caster->Refs || caster->Destroyed);
	    caster->Refs++;
	}
    }
    PlayGameSound(spell->SoundWhenCast.Sound, MaxSampleVolume);
    caster->Mana -= spell->ManaCost;
    return caster->Mana > spell->ManaCost;
}

/**
**	Cast death coil.
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		=!0 if spell should be repeated, 0 if not
*/
global int CastDeathCoil(Unit* caster, const SpellType* spell, Unit* target,
    int x, int y)
{
    Missile* mis;
    int sx;
    int sy;

    DebugCheck(!caster);
    DebugCheck(!spell);
    DebugCheck(!spell->Action);
//  DebugCheck(target);
//  DebugCheck(x in range, y in range);

    mis = NULL;
    sx = caster->X;
    sy = caster->Y;

    caster->Mana -= spell->ManaCost;

    PlayGameSound(spell->SoundWhenCast.Sound, MaxSampleVolume);
    mis = MakeMissile(spell->Missile,
	sx * TileSizeX + TileSizeX / 2, sy * TileSizeY + TileSizeY / 2,
	x * TileSizeX + TileSizeX / 2, y * TileSizeY + TileSizeY / 2);
    mis->SourceUnit = caster;
    RefsDebugCheck(!caster->Refs || caster->Destroyed);
    caster->Refs++;
    if (target) {
	mis->TargetUnit = target;
	RefsDebugCheck(!target->Refs || target->Destroyed);
	target->Refs++;
    }
    mis->Controller = SpellDeathCoilController;
    return 0;
}

/**
**	Cast fireball.
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		=!0 if spell should be repeated, 0 if not
*/
global int CastFireball(Unit* caster, const SpellType* spell,
    Unit* target __attribute__((unused)), int x, int y)
{
    Missile* missile;
    int sx;
    int sy;
    int dist;

    DebugCheck(!caster);
    DebugCheck(!spell);
    DebugCheck(!spell->Action);
    DebugCheck(!spell->Missile);

    missile = NULL;

    // NOTE: fireball can be casted on spot
    sx = caster->X;
    sy = caster->Y;
    dist = MapDistance(sx, sy, x, y);
    DebugCheck(!dist);
    x += ((x - sx) * 10) / dist;
    y += ((y - sy) * 10) / dist;
    sx = sx * TileSizeX + TileSizeX / 2;
    sy = sy * TileSizeY + TileSizeY / 2;
    x = x * TileSizeX + TileSizeX / 2;
    y = y * TileSizeY + TileSizeY / 2;
    caster->Mana -= spell->ManaCost;
    PlayGameSound(spell->SoundWhenCast.Sound, MaxSampleVolume);
    missile = MakeMissile(spell->Missile, sx, sy, x, y);
    missile->State = spell->Action->Data.Fireball.TTL - (dist - 1) * 2;
    missile->TTL = spell->Action->Data.Fireball.TTL;
    missile->Damage = spell->Action->Data.Fireball.Damage;
    missile->Controller = SpellFireballController;
    missile->SourceUnit = caster;
    RefsDebugCheck(!caster->Refs || caster->Destroyed);
    caster->Refs++;
    return 0;
}

/**
**	Cast flame shield.
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		=!0 if spell should be repeated, 0 if not
*/
global int CastFlameShield(Unit* caster, const SpellType* spell, Unit* target,
    int x __attribute__((unused)), int y __attribute__((unused)))
{
    Missile* mis;
    int	i;

    DebugCheck(!caster);
    DebugCheck(!spell);
    DebugCheck(!spell->Action);
    DebugCheck(!target);
//  DebugCheck(x in range, y in range);
    DebugCheck(!spell->Missile);

    mis = NULL;

    // get mana cost
    caster->Mana -= spell->ManaCost;
    target->FlameShield = spell->Action->Data.FlameShield.TTL;
    PlayGameSound(spell->SoundWhenCast.Sound, MaxSampleVolume);
    for (i = 0; i < 5; ++i) {
	mis = MakeMissile(spell->Missile, 0, 0, 0, 0);
	mis->TTL = spell->Action->Data.FlameShield.TTL + i * 7;
	mis->TargetUnit = target;
	mis->Controller = SpellFlameShieldController;
	mis->Damage = spell->Action->Data.FlameShield.Damage;
	RefsDebugCheck(!target->Refs || target->Destroyed);
	target->Refs++;
    }
    return 0;
}

/**
**	Cast haste.
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		=!0 if spell should be repeated, 0 if not
*/
global int CastAdjustBuffs(Unit* caster, const SpellType* spell, Unit* target,
    int x, int y)
{
    DebugCheck(!caster);
    DebugCheck(!spell);
    DebugCheck(!spell->Action);
    DebugCheck(!target);

    // get mana cost
    caster->Mana -= spell->ManaCost;

    if (spell->Action->Data.AdjustBuffs.HasteTicks!=BUFF_NOT_AFFECTED) {
	target->Haste = spell->Action->Data.AdjustBuffs.HasteTicks;
    }
    if (spell->Action->Data.AdjustBuffs.SlowTicks!=BUFF_NOT_AFFECTED) {
	target->Slow = spell->Action->Data.AdjustBuffs.SlowTicks;
    }
    if (spell->Action->Data.AdjustBuffs.BloodlustTicks!=BUFF_NOT_AFFECTED) {
	target->Bloodlust = spell->Action->Data.AdjustBuffs.BloodlustTicks;
    }
    if (spell->Action->Data.AdjustBuffs.InvisibilityTicks!=BUFF_NOT_AFFECTED) {
	target->Invisible = spell->Action->Data.AdjustBuffs.InvisibilityTicks;
    }
    if (spell->Action->Data.AdjustBuffs.InvincibilityTicks!=BUFF_NOT_AFFECTED) {
	target->UnholyArmor = spell->Action->Data.AdjustBuffs.InvincibilityTicks;
    }
    CheckUnitToBeDrawn(target);
    PlayGameSound(spell->SoundWhenCast.Sound, MaxSampleVolume);
    MakeMissile(spell->Missile,
	x * TileSizeX+TileSizeX / 2, y * TileSizeY+TileSizeY / 2,
	x * TileSizeX+TileSizeX / 2, y * TileSizeY+TileSizeY / 2);
    return 0;
}

/**
**	Cast healing. (or exorcism)
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		=!0 if spell should be repeated, 0 if not
*/
global int CastAdjustVitals(Unit* caster, const SpellType* spell, Unit* target,
    int x, int y)
{
    int castcount;
    int diffHP;
    int diffMana;
    int hp;
    int mana;
    int manacost;

    DebugCheck(!caster);
    DebugCheck(!spell);
    DebugCheck(!spell->Action);
    DebugCheck(!target);

    hp = spell->Action->Data.AdjustVitals.HP;
    mana = spell->Action->Data.AdjustVitals.Mana;
    manacost = spell->ManaCost;

    //  Healing and harming
    if (hp > 0) {
	diffHP = target->Stats->HitPoints - target->HP;
    } else {
	diffHP = target->HP;
    }
    if (mana > 0) {
	diffMana = target->Type->_MaxMana - target->Mana;
    } else {
	diffMana = target->Mana;
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
	castcount = min(castcount, caster->Mana / manacost);
    }
    if (spell->Action->Data.AdjustVitals.MaxMultiCast) {
	castcount = min(castcount, spell->Action->Data.AdjustVitals.MaxMultiCast);
    }

    DebugCheck(castcount < 0);

    DebugLevel3Fn("Used to have %d hp and %d mana.\n" _C_
	target->HP _C_ target->Mana);

    caster->Mana -= castcount * manacost;
    if (hp < 0) {
	HitUnit(caster, target, castcount * hp);
    } else {
	target->HP += castcount * hp;
	if (target->HP > target->Stats->HitPoints) {
	    target->HP = target->Stats->HitPoints;
	}
    }
    target->Mana += castcount*mana;
    if (target->Mana < 0) {
	target->Mana = 0;
    }
    if (target->Mana > target->Type->_MaxMana) {
	target->Mana = target->Type->_MaxMana;
    }

    DebugLevel3Fn("Unit now has %d hp and %d mana.\n" _C_
	target->HP _C_ target->Mana);
    PlayGameSound(spell->SoundWhenCast.Sound, MaxSampleVolume);
    MakeMissile(spell->Missile,
	x * TileSizeX + TileSizeX / 2, y * TileSizeY + TileSizeY / 2,
	x * TileSizeX + TileSizeX / 2, y * TileSizeY + TileSizeY / 2);
    return 0;
}

/**
**	Cast polymorph.
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		=!0 if spell should be repeated, 0 if not
*/
global int CastPolymorph(Unit* caster, const SpellType* spell, Unit* target,
    int x, int y)
{
    UnitType* type;

    DebugCheck(!caster);
    DebugCheck(!spell);
    DebugCheck(!spell->Action);
    DebugCheck(!target);

    type = spell->Action->Data.Polymorph.NewForm;
    DebugCheck(!type);

    caster->Player->Score += target->Type->Points;
    if (target->Type->Building) {
	caster->Player->TotalRazings++;
    } else {
	caster->Player->TotalKills++;
    }
#ifdef USE_HP_FOR_XP
    caster->XP += target->HP;
#else
    caster->XP += target->Type->Points;
#endif
    caster->Kills++;
    // as said somewhere else -- no corpses :)
    RemoveUnit(target, NULL);
    UnitLost(target);
    UnitClearOrders(target);
    ReleaseUnit(target);
    if (UnitTypeCanMoveTo(x, y, type)) {
	MakeUnitAndPlace(x, y, type, Players + PlayerNumNeutral);
    }
    caster->Mana -= spell->ManaCost;
    PlayGameSound(spell->SoundWhenCast.Sound, MaxSampleVolume);
    MakeMissile(spell->Missile,
	x * TileSizeX + TileSizeX / 2, y * TileSizeY + TileSizeY / 2,
	x * TileSizeX + TileSizeX / 2, y * TileSizeY + TileSizeY / 2);
    return 0;
}

/**
**	Cast runes.
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		=!0 if spell should be repeated, 0 if not
*/
global int CastRunes(Unit* caster, const SpellType* spell,
    Unit* target __attribute__((unused)), int x, int y)
{
    Missile* mis;
    const int xx[] = {-1,+1, 0, 0, 0};
    const int yy[] = { 0, 0, 0,-1,+1};
    int oldx;
    int oldy;
    int i;

    DebugCheck(!caster);
    DebugCheck(!spell);
    DebugCheck(!spell->Action);
//  DebugCheck(x in range, y in range);

    mis = NULL;
    oldx = x;
    oldy = y;

    PlayGameSound(spell->SoundWhenCast.Sound, MaxSampleVolume);
    for (i = 0; i < 5; ++i) {
	x = oldx + xx[i];
	y = oldy + yy[i];
	    
	if (IsMapFieldEmpty(x, y)) {
	    mis = MakeMissile(spell->Missile,
		x * TileSizeX + TileSizeX / 2,
		y * TileSizeY + TileSizeY / 2,
		x * TileSizeX + TileSizeX / 2,
		y * TileSizeY + TileSizeY / 2);
	    mis->TTL = spell->Action->Data.Runes.TTL;
 	    mis->Damage = spell->Action->Data.Runes.Damage;
 	    mis->SourceUnit = caster;
	    caster->Mana -= spell->ManaCost / 5;
	}
    }
    return 0;
}

/**
**	Cast summon spell.
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		=!0 if spell should be repeated, 0 if not
*/
global int CastSummon(Unit* caster, const SpellType* spell, Unit* target,
    int x, int y)
{
    int ttl;
    int cansummon;
    Unit** corpses;
    Unit* tempcorpse;
    UnitType* unittype;

    DebugCheck(!caster);
    DebugCheck(!spell);
    DebugCheck(!spell->Action);
    DebugCheck(!spell->Action->Data.Summon.UnitType);

    unittype = spell->Action->Data.Summon.UnitType;
    ttl = spell->Action->Data.Summon.TTL;

    if (spell->Action->Data.Summon.RequireCorpse) {
	corpses = &CorpseList;
	cansummon = 0;
	while (*corpses) {
	    // FIXME: this tries to raise all corps, I can raise ships?
	    if ((*corpses)->Orders[0].Action == UnitActionDie &&
		    !(*corpses)->Type->Building &&
		    (*corpses)->X >= x - 1 && (*corpses)->X <= x + 1 &&
		    (*corpses)->Y >= y - 1 && (*corpses)->Y <= y + 1) {
		//
		//  Found a corpse. eliminate it and proceed to summoning.
		//  
		x = (*corpses)->X;
		y = (*corpses)->Y;
		tempcorpse = *corpses;
		corpses = &(*corpses)->Next;
		ReleaseUnit(tempcorpse);
		cansummon = 1;
		break;
	    } else {
		corpses = &(*corpses)->Next;
	    }
	}
    } else { 
	cansummon = 1;
    }

    if (cansummon) {
	DebugLevel0("Summoning a %s\n" _C_ unittype->Name);

	//
	//	Create units.
	//	FIXME: do summoned units count on food?
	//
	target = MakeUnit(unittype, caster->Player);
	target->X = x;
	target->Y = y;
	//
	//  set life span. ttl=0 results in a permanent unit.
	//  
	if (ttl) {
	    target->TTL = GameCycle + ttl;
	}
	//
	//	Revealers are always removed, since they don't have graphics
	//
	if (target->Type->Revealer) {
	    DebugLevel0Fn("summoned unit is a revealer, removed.\n");
	    target->Removed = 1;
	    target->CurrentSightRange = target->Stats->SightRange;
	    MapMarkUnitSight(target);
	} else {
	    DropOutOnSide(target, LookingW, 0, 0);
	    CheckUnitToBeDrawn(target);
	}
	
	caster->Mana -= spell->ManaCost;
    }

    PlayGameSound(spell->SoundWhenCast.Sound, MaxSampleVolume);
    MakeMissile(spell->Missile,
	x * TileSizeX + TileSizeX / 2, y * TileSizeY + TileSizeY / 2,
	x * TileSizeX + TileSizeX / 2, y * TileSizeY + TileSizeY / 2);
    return 0;
}

/**
**	Cast whirlwind.
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		=!0 if spell should be repeated, 0 if not
*/
global int CastWhirlwind(Unit* caster, const SpellType* spell,
    Unit* target __attribute__((unused)), int x, int y)
{
    Missile* mis;

    DebugCheck(!caster);
    DebugCheck(!spell);
    DebugCheck(!spell->Action);
//  DebugCheck(x in range, y in range);

    mis = NULL;

    caster->Mana -= spell->ManaCost;
    PlayGameSound(spell->SoundWhenCast.Sound, MaxSampleVolume);
    mis = MakeMissile(spell->Missile,
	x * TileSizeX + TileSizeX / 2, y * TileSizeY + TileSizeY / 2,
	x * TileSizeX + TileSizeX / 2, y * TileSizeY + TileSizeY / 2);
    mis->TTL = spell->Action->Data.Whirlwind.TTL;
    mis->Controller = SpellWhirlwindController;
    return 0;
}

// ****************************************************************************
// Target constructor
// ****************************************************************************

/**
**	FIXME: docu
*/
local Target* NewTarget(TargetType t, const Unit* unit, int x, int y)
{
    Target* target;

    target = (Target*)malloc(sizeof(*target));

    DebugCheck(unit == NULL && t == TargetUnit);
    DebugCheck(!(0 <= x && x < TheMap.Width) && t == TargetPosition);
    DebugCheck(!(0 <= y && y < TheMap.Height) && t == TargetPosition);

    target->which_sort_of_target = t;
    target->unit = (Unit*)unit;
    target->X = x;
    target->Y = y;
    return target;
}

/**
**	FIXME: docu
*/
local Target* NewTargetUnit(const Unit* unit)
{
    DebugCheck(!unit);
    return NewTarget(TargetUnit, unit, 0, 0);
}

/**
**	FIXME: docu
*/
local Target* NewTargetPosition(int x, int y)
{
    DebugCheck(!(0 <= x && x < TheMap.Width));
    DebugCheck(!(0 <= y && y < TheMap.Height));

    return NewTarget(TargetPosition, NULL, x, y);
}

// ****************************************************************************
//	Main local functions
// ****************************************************************************

/*
**	Check the condition.
**
**	@param caster		Pointer to caster unit.
**	@param spell 		Pointer to the spell to cast.
**	@param target		Pointer to target unit, or 0 if it is a position spell.
**	@param x		X position, or -1 if it is an unit spell.
**	@param y		Y position, or -1 if it is an unit spell.
**	@param condition	Pointer to condition info.
**
**	@return 1 if passed, 0 otherwise.
*/
local int PassCondition(const Unit* caster, const SpellType* spell, const Unit* target,
    int x, int y, const ConditionInfo* condition)
{
    //
    //	Check caster mana. FIXME: move somewhere else?
    //
    if (caster->Mana < spell->ManaCost) {
	return 0;
    }
    //
    //	Casting an unit spell without a target. 
    //
    if (spell->Target == TargetUnit && !target) {
	return 0;
    }
    if (!condition) {
	// no condition, pass.
	return 1;
    }
    //
    //	Now check conditions regarding the target unit.
    //
    if (target) {
	if (condition->Undead != CONDITION_TRUE) {
	    if ((condition->Undead == CONDITION_ONLY) ^ (target->Type->IsUndead)) {
		return 0;
	    }
	}
	if (condition->Organic != CONDITION_TRUE) {
	    if ((condition->Organic == CONDITION_ONLY) ^ (target->Type->Organic)) {
		return 0;
	    }
	}
	if (condition->Volatile != CONDITION_TRUE) {
	    if ((condition->Volatile == CONDITION_ONLY) ^ (target->Type->Volatile)) {
		return 0;
	    }
	}
	if (condition->Building != CONDITION_TRUE) {
	    if ((condition->Building == CONDITION_ONLY) ^ (target->Type->Building)) {
		return 0;
	    }
	}
	if (condition->Hero != CONDITION_TRUE) {
	    if ((condition->Hero == CONDITION_ONLY) ^ (target->Type->Hero)) {
		return 0;
	    }
	}
	if (condition->Coward != CONDITION_TRUE) {
	    if ((condition->Coward == CONDITION_ONLY) ^ (target->Type->Coward)) {
		return 0;
	    }
	}
	if (condition->Alliance != CONDITION_TRUE) {
	    if ((condition->Alliance == CONDITION_ONLY) ^ 
		    (IsAllied(caster->Player,target) || target->Player == caster->Player)) {
		return 0;
	    }
	}
	if (condition->TargetSelf != CONDITION_TRUE) {
	    if ((condition->TargetSelf == CONDITION_ONLY) ^ (caster == target)) {
		return 0;
	    }
	}
	//
	//	Check vitals now.
	//
	if (condition->MinHpPercent * target->Stats->HitPoints / 100 > target->HP) {
	    return 0;
	}
	if (condition->MaxHpPercent * target->Stats->HitPoints / 100 <= target->HP) {
	    return 0;
	}
	if (target->Type->CanCastSpell) {
	    if (condition->MinManaPercent * target->Type->_MaxMana / 100 > target->Mana) {
		return 0;
	    }
	    if (condition->MaxManaPercent * target->Type->_MaxMana / 100 < target->Mana) {
		return 0;
	    }
	}
	//
	//	Check for slow/haste stuff
	//	This should be used mostly for ai, if you want to keep casting
	//	slow to no effect I can't see why should we stop you.
	//
	if (condition->MaxSlowTicks < target->Slow) {
	    return 0;
	}
	if (condition->MaxHasteTicks < target->Haste) {
	    return 0;
	}
	if (condition->MaxBloodlustTicks < target->Bloodlust) {
	    return 0;
	}
	if (condition->MaxInvisibilityTicks < target->Invisible) {
	    return 0;
	}
	if (condition->MaxInvincibilityTicks < target->UnholyArmor) {
	    return 0;
	}
    }
    return 1;
}

/**
**	Select the target for the autocast.
**
**	@param caster	Unit who would cast the spell.
**	@param spell	Spell-type pointer.
**	
**	@return Target*	choosen target or Null if spell can't be cast.
**
*/
// FIXME: should be global (for AI) ???
local Target* SelectTargetUnitsOfAutoCast(const Unit* caster, const SpellType* spell)
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

    DebugCheck(!spell);
    DebugCheck(!spell->AutoCast);
    DebugCheck(!caster);

    //
    //	Ai cast should be a lot better. Use autocast if not found.
    //
    if (caster->Player->Ai && spell->AICast) {
	DebugLevel3Fn("The borg uses AI autocast XP.\n");
	autocast = spell->AICast;
    } else {
	DebugLevel3Fn("You puny mortal, join the colective!\n");
	autocast = spell->AutoCast;
    }

    x = caster->X;
    y = caster->Y;
    range = spell->AutoCast->Range;

    //
    //	Select all units aroung the caster
    //
    nunits = SelectUnits(caster->X - range, caster->Y - range,
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
    //	Check generic conditions. FIXME: a better way to do this?
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
	    //	The units are already selected.
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
	    //	Now select the best unit to target.
	    //	FIXME: Some really smart way to do this. 
	    //	FIXME: Heal the unit with the lowest hit-points
	    //	FIXME: Bloodlust the unit with the highest hit-point
	    //	FIMXE: it will survive more
	    //
	    if (nunits != 0) {
#if 0
		// For the best target???
		sort(table, nb_units, spell->autocast->f_order);
		return NewTargetUnit(table[0]);
#else
		//	Best unit, random unit, oh well, same stuff.
		i = SyncRand() % nunits;
		return NewTargetUnit(table[i]);
#endif
	    }
	    break;
	default:
	    // Something is wrong
	    DebugLevel0Fn("Spell is screwed up, unknown target type\n");
	    DebugCheck(1);
	    return NULL;
	    break;
	}
    return NULL;	// Can't spell the auto-cast.
}

// ****************************************************************************
//	Public spell functions
// ****************************************************************************

// ****************************************************************************
// Constructor and destructor
// ****************************************************************************

/**
**	Spells constructor, inits spell id's and sounds
*/
global void InitSpells(void)
{
}

// ****************************************************************************
// Get Spell.
// ****************************************************************************

/**
**	Get the numeric spell id by string identifer.
**
**	@param IdentName	Spell identifier
**
**	@return		Spell id (index in spell-type table)
*/
global int SpellIdByIdent(const char* ident)
{
    int id;

    DebugCheck(!ident);

    for (id = 0; id < SpellTypeCount; ++id) {
	if (strcmp(SpellTypeTable[id].IdentName, ident) == 0) {
	    return id;
	}
    }
    return -1;
}

/**
**	Get spell-type struct pointer by string identifier.
**
**	@param IdentName	Spell identifier.
**
**	@return		spell-type struct pointer.
*/
global SpellType* SpellTypeByIdent(const char* ident)
{
    int id;

    DebugCheck(!ident);

    id = SpellIdByIdent(ident);
    return (id == -1 ? NULL : &SpellTypeTable[id]);
}

/**
**	FIXME: docu
*/
global unsigned CclGetSpellByIdent(SCM value)
{  
    int i;

    for (i = 0; i < SpellTypeCount; ++i) {
	if (gh_eq_p(value, gh_symbol2scm(SpellTypeTable[i].IdentName))) {
	    return i;
	}
    }
    return 0xABCDEF;
}

/**
**	Get spell-type struct ptr by id
**
**	@param id  Spell id (index in the spell-type table)
**
**	@return spell-type struct ptr
*/
global SpellType* SpellTypeById(int id)
{
    DebugCheck(!(0 <= id && id < SpellTypeCount));
    return &SpellTypeTable[id];
}

// ****************************************************************************
// CanAutoCastSpell, CanCastSpell, AutoCastSpell, CastSpell.
// ****************************************************************************

/**
**	Check if spell is research for player \p player.
**	@param	player : player for who we want to know if he knows the spell.
**	@param	id : 
*/
global int SpellIsAvailable(const Player* player, int spellid)
{
    int dependencyId;
    
    DebugCheck(!player);
    DebugCheck(!(0 <= spellid && spellid < SpellTypeCount));

    dependencyId = SpellTypeTable[spellid].DependencyId;

    return dependencyId == -1 || UpgradeIdAllowed(player, dependencyId) == 'R';
}


/**
**	Check if the spell can be auto cast.
**
**	@param spell	Spell-type pointer
**
**	@return		1 if spell can be cast, 0 if not
*/
global int CanAutoCastSpell(const SpellType* spell)
{
    DebugCheck(!spell);

    return spell->AutoCast ? 1 : 0;
}

/**
**	Check if unit can cast the spell.
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		=!0 if spell should/can casted, 0 if not
*/
global int CanCastSpell(const Unit* caster, const SpellType* spell,
    const Unit* target, // FIXME : Use a unique struture t_Target ?
    int x, int y)
{
    DebugCheck(!caster);
    DebugCheck(!spell);

    // And caster must know the spell
    // FIXME spell->Ident < MaxSpell
    DebugCheck(!(caster->Type->CanCastSpell && caster->Type->CanCastSpell[spell->Ident]));

    if (!caster->Type->CanCastSpell ||
	    !caster->Type->CanCastSpell[spell->Ident] ||
	    (spell->Target == TargetUnit && target == NULL)) {
	return 0;
    }

    return PassCondition(caster, spell, target, x, y, spell->Condition);
}

/**
**	Check if the spell can be auto cast and cast it.
**
**	@param caster	Unit who can cast the spell.
**	@param spell	Spell-type pointer.
**
**	@return		1 if spell is casted, 0 if not.
*/
global int AutoCastSpell(Unit* caster, const SpellType* spell)
{
    Target* target;

    DebugCheck(!caster);
    DebugCheck(!spell);
    DebugCheck(!(0 <= spell->Ident && spell->Ident < SpellTypeCount));
    DebugCheck(!(caster->Type->CanCastSpell));
    DebugCheck(!(caster->Type->CanCastSpell[spell->Ident]));

    target = NULL;

    //  Check for mana, trivial optimization.
    if (caster->Mana < spell->ManaCost) {
	return 0;
    }
    target = SelectTargetUnitsOfAutoCast(caster, spell);
    if (target == NULL) {
	return 0;
    } else {
	//	Must move before ?
	//	FIXME SpellType* of CommandSpellCast must be const.
	CommandSpellCast(caster, target->X, target->Y, target->unit,
	    (SpellType*)spell, FlushCommands);
	free(target);
    }
    return 1;
}

/**
**	Spell cast!
**
**	@param caster	Unit that casts the spell
**	@param spell	Spell-type pointer
**	@param target	Target unit that spell is addressed to
**	@param x	X coord of target spot when/if target does not exist
**	@param y	Y coord of target spot when/if target does not exist
**
**	@return		!=0 if spell should/can continue or 0 to stop
*/
global int SpellCast(Unit* caster, const SpellType* spell, Unit* target,
    int x, int y)
{
    DebugCheck(!spell);
    DebugCheck(!spell->Action->CastFunction);
    DebugCheck(!caster);
    DebugCheck(!SpellIsAvailable(caster->Player, spell->Ident));

    caster->Invisible = 0;// unit is invisible until attacks // FIXME Must be configurable
    if (target) {
	x = target->X;
	y = target->Y;
    } else {
	x += spell->Range;	// Why ??
	y += spell->Range;	// Why ??
    }
    DebugLevel3Fn("Spell cast: (%s), %s -> %s (%d,%d)\n" _C_ spell->IdentName _C_
	unit->Type->Name _C_ target ? target->Type->Name : "none" _C_ x _C_ y);
    return CanCastSpell(caster, spell, target, x, y) &&
	spell->Action->CastFunction(caster, spell, target, x, y);
}

/*
**	Cleanup the spell subsystem.
**	
**	@note: everything regarding spells is gone now.
**	FIXME: not complete
*/
void CleanSpells(void)
{
    SpellType* spell;

    DebugLevel0("Cleaning spells.\n");
    for (spell = SpellTypeTable; spell < SpellTypeTable + SpellTypeCount; ++spell) {
	free(spell->IdentName);
	free(spell->Name);
	free(spell->Action);
	if (spell->Condition) {
	    free(spell->Condition);
	}
	//
	//	Free Autocast.
	//
	if (spell->AutoCast) {
	    if (spell->AutoCast->Condition) {
		free(spell->AutoCast->Condition);
	    }
	    free(spell->AutoCast);
	}
	if (spell->AICast) {
	    if (spell->AICast->Condition) {
		free(spell->AICast->Condition);
	    }
	    free(spell->AICast);
	}
	// FIXME: missile free somewhere else, right?
    }
    free(SpellTypeTable);
    SpellTypeTable = 0;
    SpellTypeCount = 0;
}

#if 0

/*
**	 TODO :
**	- Modify missile.c for better configurable and clear the code.
** ccl info


// !!! Special deathcoil

// if (!target->Type->Building
	   && (target->Type->UnitType == UnitTypeLand || target->Type->UnitType == UnitTypeNaval)
	&& target->FlameShield < spell->TTL) // FlameShield

	= {

  NOTE: vladi:

  The point to have variable unsorted list of spell-types and
  dynamic id's and in the same time -- SpellAction id's is that
  spell actions are hardcoded and cannot be changed at all.
  On the other hand we can have different spell-types as with
  different range, cost and time to live (possibly and other
  parameters as extensions)

*/

#endif

//@}
