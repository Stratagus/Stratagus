//       _________ __                 __                               
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ \ 
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/ 
//  ______________________                           ______________________
//			  T H E   W A R   B E G I N S
//	   Stratagus - A free fantasy real time strategy game engine
//
/**@name sound_id.h	-	Sound identifier client side header file. */
//
//	(c) Copyright 1999-2002 by Lutz Sammer and Fabrice Rossi
//
//	Stratagus is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published
//	by the Free Software Foundation; only version 2 of the License.
//
//	Stratagus is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	$Id$

#ifndef __SOUND_ID_H__
#define __SOUND_ID_H__

//@{

/*----------------------------------------------------------------------------
--	Declarations
----------------------------------------------------------------------------*/

/**
**	Sound referencing.
**
**	Client side representation of the sound id.
**	Passed to the sound server API.
*/
typedef void* SoundId;			/// sound identifier

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

    /// Make a sound bound to identifier
extern SoundId MakeSound(const char* sound_name,char* file[], int nb);
    /// Get the sound id bound to an identifier
extern SoundId SoundIdForName(const char* sound_name);
    /// Map sound to identifier
extern void MapSound(const char* sound_name,const SoundId id);
    /// Make a sound group bound to identifier
extern SoundId MakeSoundGroup(const char* name,SoundId first,SoundId second);
    /// Helper function, displays the mapping between sound names and sound ids
extern void DisplaySoundHashTable(void);

#ifndef WITH_SOUND	// {

#define SoundIdForName(n)	NULL	/// Dummy macro for without sound

#endif	// } WITH_SOUND

//@}

#endif	// !__SOUND_ID_H__
