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
/**@name netconnect.h	-	The network connection setup header file. */
//
//	(c) Copyright 1998-2001 by Lutz Sammer, Andreas Arens
//
//	$Id$

#ifndef __NETCONNECT_H__
#define __NETCONNECT_H__

//@{

/*----------------------------------------------------------------------------
--	Defines
----------------------------------------------------------------------------*/

    /// Network protocol major version
#define NetworkProtocolMajorVersion	0
    /// Network protocol minor version (maximal 99)
#define NetworkProtocolMinorVersion	3
    /// Network protocol patch level (maximal 99)
#define NetworkProtocolPatchLevel	2
    /// Network protocol version (1,2,3) -> 10203
#define NetworkProtocolVersion \
	(NetworkProtocolMajorVersion*10000+NetworkProtocolMinorVersion*100 \
	+NetworkProtocolPatchLevel)

    /// Network protocol printf format string
#define NetworkProtocolFormatString	"%d,%d,%d"
    /// Network protocol printf format arguments
#define NetworkProtocolFormatArgs(v)	(v)/10000,((v)/100)%100,(v)%100

#define NetworkDefaultPort	6660	/// Default communication port

/*----------------------------------------------------------------------------
--	Declarations
----------------------------------------------------------------------------*/

/**
**	Network systems active in current game.
*/
typedef struct _network_host_ {
    unsigned long	Host;		/// Host address
    unsigned short	Port;		/// Port on host
    unsigned short	PlyNr;		/// Player nummer
    char		PlyName[16];	/// Name of player
} NetworkHost;

/**
**	Network init message.
*/
typedef struct _init_message_ {
    unsigned char  Type;		/// Init message type.
    unsigned char  SubType;		/// Init message subtype.
    int		   FreeCraft;		/// FreeCraft engine version.
    int		   Version;		/// Network protocol version.
    unsigned int   ConfUID;		/// Engine configuration UID (Checksum)	// FIXME: not available yet
    unsigned int   MapUID;		/// UID of map to play.	// FIXME: add MAP name, path, etc
    int		   Lag;			/// Lag time
    int		   Updates;		/// Update frequency
    char	   HostsCount;		/// Number of hosts.
    
    NetworkHost	   Hosts[PlayerMax];	/// Participant information.
} InitMessage;

/**
**	Network init config message subtypes (menu state machine).
*/
enum _ic_message_subtype_ {
    ICMHello,				/// Client Request
    ICMConfig,				/// Setup message configure clients

    ICMEngineMismatch,			/// FreeCraft engine version doesn't match
    ICMProtocolMismatch,		/// Network protocol version doesn't match
    ICMEngineConfMismatch,		/// Engine configuration isn't identical
    ICMMapUidMismatch,			/// MAP Uid  doesn't match

    ICMGameFull,			/// No player slots available

    ICMServerQuit,			/// Server has quit game
};

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

extern char* NetworkArg;		/// Network command line argument
extern int NetPlayers;			/// Network players
extern int NetworkPort;			/// Local network port to use
extern char NetworkName[16];		/// Network Name of local player

extern int HostsCount;			/// Number of hosts.
extern NetworkHost Hosts[PlayerMax];	/// Host, port, and number of all players.

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

extern void NetworkServerSetup(WorldMap *map);	/// connection server setup
extern void NetworkClientSetup(WorldMap *map);	/// connection client setup
extern void NetworkSetupArgs(void);		/// setup command line connection parameters
extern void NetworkParseSetupEvent(const char *buf, int size); /// parse a setup event

//@}

#endif	// !__NETCONNECT_H__
