#include "../idlib/precompiled.h"
#pragma hdrstop

static idBitMsg outMsg;
static char msgBuf[ MAX_GAME_MESSAGE_SIZE ];

#define BUFF_INIT\
	outMsg.Init( msgBuf, sizeof( msgBuf ) );\
	outMsg.BeginWriting();

#define BUFF_SEND( client )\
	networkSystem->ServerSendReliableMessage( client, outMsg );

static int parse_cmd( const char *cmd )
{
	
}

void terminal_cmd( const int client, const char *text, mpPlayerState_t *playerState )
{
	idEntity	*ent;
	idPlayer	*player;
	int i;
	// whoami, list, position, help
	//
}
