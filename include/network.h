//   ___________		     _________		      _____  __
//   \_	  _____/______   ____   ____ \_   ___ \____________ _/ ____\/  |_
//    |    __) \_  __ \_/ __ \_/ __ \/    \  \/\_  __ \__  \\   __\\   __\ 
//    |     \   |  | \/\  ___/\  ___/\     \____|  | \// __ \|  |   |  |
//    \___  /   |__|    \___  >\___  >\______  /|__|  (____  /__|   |__|
//	  \/		    \/	   \/	     \/		   \/
//  ______________________                           ______________________
//			  T H E   W A R   B E G I N S
//	   FreeCraft - A free fantasy real time strategy game engine
//
/**@name network.h	-	The network header file. */
//
//	(c) Copyright 1998-2002 by Lutz Sammer
//
//	FreeCraft is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published
//	by the Free Software Foundation; only version 2 of the License.
//
//	FreeCraft is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	$Id$

#ifndef __NETWORK_H__
#define __NETWORK_H__

//@{

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include "unittype.h"
#include "unit.h"
#include "upgrade.h"

/*----------------------------------------------------------------------------
--	Defines
----------------------------------------------------------------------------*/

#define NetworkMaxLag	250		/// Debuging network lag (# game cycles)

#define NetworkDups	4		/// Repeat old commands

/*----------------------------------------------------------------------------
--	Declarations
----------------------------------------------------------------------------*/

/**
**	Network message types.
*/
enum _message_type_ {
    MessageInitHello,			/// start connection
    MessageInitReply,			/// connection reply
    MessageInitConfig,			/// setup message configure clients

    MessageSync,			/// heart beat
    MessageQuit,			/// quit game
    MessageQuitAck,			/// quit reply - UNUSED YET	Protocol Version 2 - Reserved for menus
    MessageResend,			/// resend message

    MessageChat,			/// chat message
    MessageChatTerm,			/// chat message termination -  Protocol Version 2

    MessageCommandStop,			/// unit command stop
    MessageCommandStand,		/// unit command stand ground
    MessageCommandFollow,		/// unit command follow
    MessageCommandMove,			/// unit command move
    MessageCommandRepair,		/// unit command repair
    MessageCommandAttack,		/// unit command attack
    MessageCommandGround,		/// unit command attack ground
    MessageCommandPatrol,		/// unit command patrol
    MessageCommandBoard,		/// unit command borad
    MessageCommandUnload,		/// unit command unload
    MessageCommandBuild,		/// unit command build building
    MessageCommandCancelBuild,		/// unit command cancel building
    MessageCommandHarvest,		/// unit command harvest
    MessageCommandMine,			/// unit command mine gold
    MessageCommandHaul,			/// unit command haul oil
    MessageCommandReturn,		/// unit command return goods
    MessageCommandTrain,		/// unit command train
    MessageCommandCancelTrain,		/// unit command cancel training
    MessageCommandUpgrade,		/// unit command upgrade
    MessageCommandCancelUpgrade,	/// unit command cancel upgrade
    MessageCommandResearch,		/// unit command research
    MessageCommandCancelResearch,	/// unit command cancel research
    MessageCommandDemolish,		/// unit command demolish

    // ATTN: __MUST__ be last due to spellid encoding!!!
    MessageCommandSpellCast		/// unit command spell cast
};

/**
**	Network acknowledge message.
*/
typedef struct _ack_message_ {
    unsigned char	Type;		/// Acknowledge message type.
} Acknowledge;

/**
**	Network command message.
*/
typedef struct _network_command_ {
    unsigned char	Type;		/// Network command type.
    unsigned char	Cycle;		/// Destination game cycle.
    UnitRef		Unit;		/// Command for unit.
    unsigned short	X;		/// Map position X.
    unsigned short	Y;		/// Map position Y.
    UnitRef		Dest;		/// Destination unit.
} NetworkCommand;

/**
**	Network packet.
**
**	This is sent over the network.
*/
typedef struct _network_packet_ {
					/// Commands in packet.
    NetworkCommand	Commands[NetworkDups];
} NetworkPacket;

/**
**	Network chat message.
*/
typedef struct _network_chat_ {
    unsigned char	Cycle;		/// Destination game cycle
    unsigned char	Type;		/// Network command type
    unsigned char	Player;		/// Sending player
    char		Text[7];	/// Message bytes
} NetworkChat;

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

extern int NetworkNumInterfaces;	/// Network number of interfaces
extern int NetworkFildes;		/// Network file descriptor
extern int NetworkInSync;		/// Network is in sync
extern int NetworkUpdates;		/// Network update each # game cycles
extern int NetworkLag;			/// Network lag (# game cycles)
extern int NetworkStatus[PlayerMax];	/// Network status

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

extern void InitNetwork1(void);		/// initialise network part 1 (ports)
extern void InitNetwork2(void);		/// initialise network part 2
extern void ExitNetwork1(void);		/// cleanup network part 1 (ports)
extern void NetworkEvent(void);		/// handle network events
extern void NetworkSync(void);		/// hold in sync
extern void NetworkQuit(void);		/// quit game
extern void NetworkRecover(void);	/// Recover network
extern void NetworkCommands(void);	/// get all network commands
extern void NetworkChatMessage(const char*msg);	/// send chat message
extern void NetworkSendCommand(int command,const Unit* unit,int x,int y
	,const Unit* dest,const UnitType* type,int status);

//@}

#endif	// !__NETWORK_H__
