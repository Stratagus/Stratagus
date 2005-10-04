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
/**@name icons.cpp - The icons. */
//
//      (c) Copyright 1998-2005 by Lutz Sammer and Jimmy Salmon
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

#include "stratagus.h"

#include <string>
#include <vector>
#include <map>

#include "map.h"
#include "video.h"
#include "icons.h"
#include "player.h"
#include "script.h"
#include "ui.h"
#include "menus.h"


/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

static std::vector<CIcon *> AllIcons;          /// Vector of all icons.
std::map<std::string, CIcon *> Icons;          /// Map of ident to icon.


/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  CIcon constructor
*/
CIcon::CIcon(const char *ident) : G(NULL), Frame(0)
{
	Ident = new_strdup(ident);
}

/**
**  CIcon destructor
*/
CIcon::~CIcon()
{
	delete[] this->Ident;
	CGraphic::Free(this->G);
}

/**
**  Create a new icon
**
**  @param ident  Icon identifier
*/
CIcon *CIcon::New(const char *ident)
{
	CIcon *icon = Icons[ident];
	if (icon) {
		return icon;
	} else {
		icon = new CIcon(ident);
		Icons[ident] = icon;
		AllIcons.push_back(icon);
		return icon;
	}
}

/**
**  Get an icon
**
**  @param ident  Icon identifier
*/
CIcon *CIcon::Get(const char *ident)
{
	CIcon *icon = Icons[ident];
	if (!icon) {
		DebugPrint("icon not found: %s" _C_ ident);
	}
	return icon;
}

/**
**  Init the icons.
**
**  Add the short name and icon aliases to hash table.
*/
void InitIcons(void)
{
}

/**
**  Load the graphics for the icons.
*/
void LoadIcons(void)
{
	for (std::vector<CIcon *>::size_type i = 0; i < AllIcons.size(); ++i) {
		CIcon *icon = AllIcons[i];
		icon->G->Load();
		ShowLoadProgress("Icons %s", icon->G->File);
		if (icon->Frame >= icon->G->NumFrames) {
			DebugPrint("Invalid icon frame: %s - %d\n" _C_
				icon->GetIdent() _C_ icon->Frame);
			icon->Frame = 0;
		}
	}
}

/**
**  Clean up memory used by the icons.
*/
void CleanIcons(void)
{
	for (std::vector<CIcon *>::iterator i = AllIcons.begin();
		i != AllIcons.end();
		++i) {
		delete *i;
	}
	AllIcons.clear();
	Icons.clear();
}

/**
**  Find the icon by identifier.
**
**  @param ident  The icon identifier.
**
**  @return       Icon pointer or NULL if not found.
*/
CIcon *IconByIdent(const char *ident)
{
	CIcon *icon = Icons[ident];
	if (!icon) {
		DebugPrint("Icon %s not found\n" _C_ ident);
	}
	return icon;
}

/**
**  Draw icon on x,y.
**
**  @param player  Player pointer used for icon colors
**  @param x       X display pixel position
**  @param y       Y display pixel position
*/
void CIcon::DrawIcon(const CPlayer *player, int x, int y) const
{
	this->G->DrawPlayerColorFrameClip(player->Index, this->Frame, x, y);
}

/**
**  Draw unit icon 'icon' with border on x,y
**
**  @param player  Player pointer used for icon colors
**  @param style   Button style
**  @param flags   State of icon (clicked, mouse over...)
**  @param x       X display pixel position
**  @param y       Y display pixel position
**  @param text    Optional text to display
*/
void CIcon::DrawUnitIcon(const CPlayer *player, ButtonStyle *style,
	unsigned flags, int x, int y, const char *text) const
{
	ButtonStyle s;

	memcpy(&s, style, sizeof(ButtonStyle));
	s.Default.Sprite = s.Hover.Sprite = s.Selected.Sprite =
		s.Clicked.Sprite = s.Disabled.Sprite = this->G;
	s.Default.Frame = s.Hover.Frame = s.Selected.Frame =
		s.Clicked.Frame = s.Disabled.Frame = this->Frame;
	if (!(flags & IconSelected) && (flags & IconAutoCast)) {
		s.Default.BorderColorRGB = UI.ButtonPanel.AutoCastBorderColorRGB;
		s.Default.BorderColor = 0;
	}
	// FIXME: player colors
	DrawMenuButton(&s, flags, x, y, text);
}

/**
**  Set an icon's ident
**
**  @param ident  New ident for icon
*/
void CIcon::SetIdent(const char *ident)
{
	Icons[this->Ident] = NULL;
	delete[] this->Ident;
	this->Ident = new_strdup(ident);
	Icons[this->Ident] = this;
}

/**
**  Parse icon definition.
**
**  @param l  Lua state.
*/
static int CclDefineIcon(lua_State *l)
{
	const char *value;
	const char *ident;
	const char *filename;
	int width;
	int height;
	int frame;

	LuaCheckArgs(l, 1);
	if (!lua_istable(l, 1)) {
		LuaError(l, "incorrect argument");
	}
	width = height = frame = 0;
	ident = filename = NULL;

	lua_pushnil(l);
	while (lua_next(l, 1)) {
		value = LuaToString(l, -2);
		if (!strcmp(value, "Name")) {
			ident = LuaToString(l, -1);
		} else if (!strcmp(value, "Size")) {
			if (!lua_istable(l, -1) || luaL_getn(l, -1) != 2) {
				LuaError(l, "incorrect argument");
			}
			lua_rawgeti(l, -1, 1);
			width = LuaToNumber(l, -1);
			lua_pop(l, 1);
			lua_rawgeti(l, -1, 2);
			height = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "File")) {
			filename = LuaToString(l, -1);
		} else if (!strcmp(value, "Frame")) {
			frame = LuaToNumber(l, -1);
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
		lua_pop(l, 1);
	}

	Assert(ident);
	Assert(!filename || (width && height));

	CIcon *icon = CIcon::New(ident);
	icon->Frame = frame;
	if (filename) {
		CGraphic::Free(icon->G);
		icon->G = CGraphic::New(filename, width, height);
	}

	return 0;
}

/**
**  Register CCL features for icons.
*/
void IconCclRegister(void)
{
	lua_register(Lua, "DefineIcon", CclDefineIcon);
}

//@}
