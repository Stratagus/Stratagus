//     ____                _       __               
//    / __ )____  _____   | |     / /___ ___________
//   / __  / __ \/ ___/   | | /| / / __ `/ ___/ ___/
//  / /_/ / /_/ (__  )    | |/ |/ / /_/ / /  (__  ) 
// /_____/\____/____/     |__/|__/\__,_/_/  /____/  
//                                              
//       A futuristic real-time strategy game.
//          This file is part of Bos Wars.
//
/**@name unittype.cpp - The unit types. */
//
//      (c) Copyright 1998-2007 by Lutz Sammer and Jimmy Salmon
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "stratagus.h"

#include <string>
#include <map>

#include "video.h"
#include "tileset.h"
#include "map.h"
#include "sound.h"
#include "unitsound.h"
#include "construct.h"
#include "unittype.h"
#include "animation.h"
#include "player.h"
#include "missile.h"
#include "script.h"
#include "spells.h"
#include "iolib.h"

#include "util.h"

#include "myendian.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CUnitType *> UnitTypes;   /// unit-types definition
std::map<std::string, CUnitType *> UnitTypeMap;

/**
**  Default names for the resources.
*/
std::string DefaultResourceNames[MaxCosts];

/**
**  Default amounts for the resources.
*/
int DefaultResourceAmounts[MaxCosts];

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Update the player stats for changed unit types.
**  @param reset indicates wether default value should be set to each stat (level, upgrades)
*/
void UpdateStats(int reset)
{
	CUnitType *type;
	CUnitStats *stats;

	//
	//  Update players stats
	//
	for (std::vector<CUnitType *>::size_type j = 0; j < UnitTypes.size(); ++j) {
		type = UnitTypes[j];
		if (reset) {
			// LUDO : FIXME : reset loading of player stats !
			for (int player = 0; player < PlayerMax; ++player) {
				stats = &type->Stats[player];
				if (!stats->Variables) {
					stats->Variables = new CVariable[UnitTypeVar.NumberVariable];
				}
				for (int i = 0; i < UnitTypeVar.NumberVariable; ++i) {
					stats->Variables[i] = type->Variable[i];
				}
			}
		}

		//
		//  As side effect we calculate the movement flags/mask here.
		//
		switch (type->UnitType) {
			case UnitTypeLand:                              // on land
				type->MovementMask =
					MapFieldLandUnit |
					MapFieldSeaUnit |
					MapFieldBuilding | // already occuppied
					MapFieldCoastAllowed |
					MapFieldWaterAllowed | // can't move on this
					MapFieldUnpassable;
				break;
			case UnitTypeFly:                               // in air
				type->MovementMask =
					MapFieldAirUnit; // already occuppied
				break;
			case UnitTypeNaval:                             // on water
				if (type->CanTransport) {
					type->MovementMask =
						MapFieldLandUnit |
						MapFieldSeaUnit |
						MapFieldBuilding | // already occuppied
						MapFieldLandAllowed; // can't move on this
					// Johns: MapFieldUnpassable only for land units?
				} else {
					type->MovementMask =
						MapFieldLandUnit |
						MapFieldSeaUnit |
						MapFieldBuilding | // already occuppied
						MapFieldCoastAllowed |
						MapFieldLandAllowed | // can't move on this
						MapFieldUnpassable;
				}
				break;
			default:
				DebugPrint("Where moves this unit?\n");
				type->MovementMask = 0;
				break;
		}
		if (type->Building || type->ShoreBuilding) {
			// Shore building is something special.
			if (type->ShoreBuilding) {
				type->MovementMask =
					MapFieldLandUnit |
					MapFieldSeaUnit |
					MapFieldBuilding | // already occuppied
					MapFieldLandAllowed; // can't build on this
			}
			type->MovementMask |= MapFieldNoBuilding;
			//
			// A little chaos, buildings without HP can be entered.
			// The oil-patch is a very special case.
			//
			if (type->Variable[HP_INDEX].Max) {
				type->FieldFlags = MapFieldBuilding;
			} else {
				type->FieldFlags = MapFieldNoBuilding;
			}
		} else switch (type->UnitType) {
			case UnitTypeLand: // on land
				type->FieldFlags = MapFieldLandUnit;
				break;
			case UnitTypeFly: // in air
				type->FieldFlags = MapFieldAirUnit;
				break;
			case UnitTypeNaval: // on water
				type->FieldFlags = MapFieldSeaUnit;
				break;
			default:
				DebugPrint("Where moves this unit?\n");
				type->FieldFlags = 0;
				break;
		}
	}
}

/**
**  Get the animations structure by ident.
**
**  @param ident  Identifier for the animation.
**
**  @return  Pointer to the animation structure.
*/
CAnimations *AnimationsByIdent(const std::string &ident)
{
	return AnimationMap[ident];
}

/**
**  Find unit-type by identifier.
**
**  @param ident  The unit-type identifier.
**
**  @return       Unit-type pointer.
*/
CUnitType *UnitTypeByIdent(const std::string &ident)
{
	return UnitTypeMap[ident];
}

/**
**  Allocate an empty unit-type slot.
**
**  @param ident  Identifier to identify the slot (malloced by caller!).
**
**  @return       New allocated (zeroed) unit-type pointer.
*/
CUnitType *NewUnitTypeSlot(const std::string &ident)
{
	CUnitType *type;

	type = new CUnitType;
	if (!type) {
		fprintf(stderr, "Out of memory\n");
		ExitFatal(-1);
	}
	type->Slot = UnitTypes.size();
	type->Ident = ident;
	type->BoolFlag = new unsigned char[UnitTypeVar.NumberBoolFlag];
	memset(type->BoolFlag, 0, UnitTypeVar.NumberBoolFlag * sizeof(unsigned char));
	type->CanTargetFlag = new unsigned char[UnitTypeVar.NumberBoolFlag];
	memset(type->CanTargetFlag, 0, UnitTypeVar.NumberBoolFlag * sizeof(unsigned char));
	type->Variable = new CVariable[UnitTypeVar.NumberVariable];
	memcpy(type->Variable, UnitTypeVar.Variable,
		UnitTypeVar.NumberVariable * sizeof(*type->Variable));

	UnitTypes.push_back(type);
	UnitTypeMap[type->Ident] = type;
	return type;
}

/**
**  Draw unit-type on map.
**
**  @param type    Unit-type pointer.
**  @param sprite  Sprite to use for drawing
**  @param player  Player number for color substitution.
**  @param frame   Animation frame of unit-type.
**  @param x       Screen X pixel postion to draw unit-type.
**  @param y       Screen Y pixel postion to draw unit-type.
**
**  @todo  Do screen position caculation in high level.
**         Better way to handle in x mirrored sprites.
*/
void DrawUnitType(const CUnitType *type, CPlayerColorGraphic *sprite, int player, int frame,
	int x, int y)
{
	// FIXME: move this calculation to high level.
	x -= (type->Width - type->TileWidth * TileSizeX) / 2;
	y -= (type->Height - type->TileHeight * TileSizeY) / 2;
	x += type->OffsetX;
	y += type->OffsetY;

	if (type->Flip) {
		if (frame < 0) {
			sprite->DrawPlayerColorFrameClipX(player, -frame - 1, x, y);
		} else {
			sprite->DrawPlayerColorFrameClip(player, frame, x, y);
		}
	} else {
		int row;

		row = type->NumDirections / 2 + 1;
		if (frame < 0) {
			frame = ((-frame - 1) / row) * type->NumDirections + type->NumDirections - (-frame - 1) % row;
		} else {
			frame = (frame / row) * type->NumDirections + frame % row;
		}
		sprite->DrawPlayerColorFrameClip(player, frame, x, y);
	}
}

/**
**  Get the still animation frame
*/
static int GetStillFrame(CUnitType *type)
{
	CAnimation *anim;

	anim = type->Animations->Still;
	while (anim) {
		if (anim->Type == AnimationFrame) {
			// Use the frame facing down
			return anim->D.Frame.Frame + type->NumDirections / 2;
		} else if (anim->Type == AnimationExactFrame) {
			return anim->D.Frame.Frame;
		}
		anim = anim->Next;
	}
	return type->NumDirections / 2;
}

/**
**  Init unit types.
*/
void InitUnitTypes(int reset_player_stats)
{
	CUnitType *type;

	for (std::vector<CUnitType *>::size_type i = 0; i < UnitTypes.size(); ++i) {
		type = UnitTypes[i];
		Assert(type->Slot == (int)i);

		//  Add idents to hash.
		UnitTypeMap[type->Ident] = UnitTypes[i];

		// Determine still frame
		type->StillFrame = GetStillFrame(type);

		// Lookup BuildingTypes
		for (std::vector<CBuildRestriction *>::iterator b = type->BuildingRules.begin();
			b < type->BuildingRules.end(); ++b) {
			(*b)->Init();
		}
	}

	// LUDO : called after game is loaded -> don't reset stats !
	UpdateStats(reset_player_stats); // Calculate the stats
}

/**
**  Loads the Sprite for a unit type
**
**  @param type  type of unit to load
*/
void LoadUnitTypeSprite(CUnitType *type)
{
	if (!type->ShadowFile.empty()) {
		type->ShadowSprite = CGraphic::ForceNew(type->ShadowFile, type->ShadowWidth,
			type->ShadowHeight);
		type->ShadowSprite->Load();
		if (type->Flip) {
			type->ShadowSprite->Flip();
		}
		type->ShadowSprite->MakeShadow();
	}

	if (!type->File.empty()) {
		type->Sprite = CPlayerColorGraphic::New(type->File, type->Width, type->Height);
		type->Sprite->Load();
		if (type->Flip) {
			type->Sprite->Flip();
		}
	}
}

/**
** Load the graphics for the unit-types.
*/
void LoadUnitTypes(void)
{
	CUnitType *type;

	for (std::vector<CUnitType *>::size_type i = 0; i < UnitTypes.size(); ++i) {
		type = UnitTypes[i];

		//
		// Lookup icons.
		//
		type->Icon.Load();
		//
		// Lookup missiles.
		//
		type->Missile.Missile = MissileTypeByIdent(type->Missile.Name);
		if (!type->Explosion.Name.empty()) {
			type->Explosion.Missile = MissileTypeByIdent(type->Explosion.Name);
		}
		//
		// Lookup corpse.
		//
		if (!type->CorpseName.empty()) {
			type->CorpseType = UnitTypeByIdent(type->CorpseName);
		}

		//
		// Load Sprite
		//
#ifndef DYNAMIC_LOAD
		if (!type->Sprite) {
			ShowLoadProgress("Unit \"%s\"", type->Name.c_str());
			LoadUnitTypeSprite(type);
		}
#endif
		// FIXME: should i copy the animations of same graphics?
	}
}

/**
**  Clean animation
*/
static void CleanAnimation(CAnimation *anim)
{
	int i;
	CAnimation *ptr;

	ptr = anim;
	while (ptr->Type != AnimationNone) {
		if (ptr->Type == AnimationSound) {
			delete[] ptr->D.Sound.Name;
		} else if (ptr->Type == AnimationRandomSound) {
			for (i = 0; i < ptr->D.RandomSound.NumSounds; ++i) {
				delete[] ptr->D.RandomSound.Name[i];
			}
			delete[] ptr->D.RandomSound.Name;
			delete[] ptr->D.RandomSound.Sound;
		}
		++ptr;
	}
	delete[] anim;
}

/**
**  Cleanup the unit-type module.
*/
void CleanUnitTypes(void)
{
	CUnitType *type;
	int j;

	DebugPrint("FIXME: icon, sounds not freed.\n");

	// FIXME: scheme contains references on this structure.
	// Clean all animations.
	for (j = 0; j < NumAnimations; ++j) {
		CleanAnimation(AnimationsArray[j]);
	}
	NumAnimations = 0;

	std::map<std::string, CAnimations *>::iterator at;
	for (at = AnimationMap.begin(); at != AnimationMap.end(); ++at) {
		delete (*at).second;
	}
	AnimationMap.clear();

	// Clean all unit-types

	for (std::vector<CUnitType *>::size_type i = 0; i < UnitTypes.size(); ++i) {
		type = UnitTypes[i];

		Assert(!type->Ident.empty());
		Assert(!type->Name.empty());

		delete[] type->Variable;
		delete[] type->BoolFlag;
		delete[] type->CanTargetFlag;
		delete[] type->CanTransport;

		for (j = 0; j < PlayerMax; j++) {
			delete[] type->Stats[j].Variables;
		}

		// Free Building Restrictions if there are any
		for (std::vector<CBuildRestriction *>::iterator b = type->BuildingRules.begin();
			b != type->BuildingRules.end(); ++b) {
			delete *b;
		}
		type->BuildingRules.clear();
		delete[] type->CanCastSpell;
		delete[] type->AutoCastActive;

		CGraphic::Free(type->Sprite);
		CGraphic::Free(type->ShadowSprite);

		delete UnitTypes[i];
	}
	UnitTypes.clear();
	UnitTypeMap.clear();

	for (j = 0; j < UnitTypeVar.NumberBoolFlag; ++j) { // User defined flags
		delete[] UnitTypeVar.BoolFlagName[j];
	}
	for (j = 0; j < UnitTypeVar.NumberVariable; ++j) { // User defined variables
		delete[] UnitTypeVar.VariableName[j];
	}
	delete[] UnitTypeVar.BoolFlagName;
	UnitTypeVar.BoolFlagName = NULL;
	UnitTypeVar.NumberBoolFlag = 0;
	delete[] UnitTypeVar.VariableName;
	UnitTypeVar.VariableName = NULL;
	delete[] UnitTypeVar.Variable;
	UnitTypeVar.Variable = NULL;
	UnitTypeVar.NumberVariable = 0;
	for (std::vector<CDecoVar *>::iterator it = UnitTypeVar.DecoVar.begin();
		it != UnitTypeVar.DecoVar.end(); ++it) {
		delete (*it);
	}
	UnitTypeVar.DecoVar.clear();
}

//@}
