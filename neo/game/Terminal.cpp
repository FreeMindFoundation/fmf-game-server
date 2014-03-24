#include "../idlib/precompiled.h"
#pragma hdrstop

#define ROOT_DIR        "/home/brick/.fmfusers/"
#define ROOT_DIR_LEN    22

#define LOWB( s ) (0x20|*s)
#define LOWW( s ) (0x2020|(*(unsigned short*)s))
#define LOWDW( s ) (0x20202020|(*(unsigned int*)s))
#define LOWQW( s ) (0x2020202020202020|(*(unsigned long long*)s))

#define _pw_ 	0x7770	// pw
#define _rm_	0x6d72  // rm
#define _cd_	0x6463 	// cd
#define _ls_	0x736c	// ls
#define _vi_	0x6976	// vi
#define _ry_	0x7972	// ry
#define _spaw_	0x77617073 // spaw
#define _sent_	0x746e6573 // sent
#define _item_	0x6d657469 // item

#define BUFF_INIT\
	outMsg.Init( msgBuf, sizeof( msgBuf ) );\
	outMsg.BeginWriting();\
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_TERMINAL );

#define BUFF_SEND( client )\
	networkSystem->ServerSendReliableMessage( client, outMsg );

#define CMPCMD( c )\
         for( end = cmd; (*end != 0x20 && *end != 0); end++ );\
         if( memcmp( cmd, c, ( end - cmd ) ) == 0 ) {\
                 mpopen( cmd, out );\
         }

enum {
	TERMINAL_STDOUT = 0,
	TERMINAL_FILES_LIST,
	TERMINAL_FILE_CONTENT,
	TERMINAL_ITEMS_LIST,
	TERMINAL_ITEM_SOURCE
};

static idBitMsg outMsg;
static byte msgBuf[ MAX_GAME_MESSAGE_SIZE ];
static int userId;
static char userDir[ 512 ];
static int userDirLen;
static int clientNum;

static void mpopen( const char *cmd, char *out )
{
  	FILE *fp;
  	char path[ 2048 ];
        int len;
        char *tmp;
	
	memset( path, 0, sizeof( path ) );

	fp = popen( cmd , "r" );
  	if( fp == NULL ) {
    		common->Printf( "Failed to run command\n" );
    		return;
  	}

        tmp = out;

  	while( fgets(path, sizeof(path)-1, fp) != NULL ) {
        	len = strlen( path );
        	memcpy( tmp, path, len );
        	tmp += len;
  	}

	pclose( fp );
}

static void create_home( void )
{
        char dir[ 128 ];

        memset( dir, 0, sizeof( dir ) );
        sprintf( dir, "mkdir -p %s%d/", ROOT_DIR, userId );
        system( dir );
}

static idPlayer *get_player()
{
	if ( gameLocal.entities[ clientNum ] && gameLocal.entities[ clientNum ]->IsType( idPlayer::Type ) ) {
		return static_cast< idPlayer * >( gameLocal.entities[ clientNum ] );
	}
	return NULL;
}

static int parse_cmd( const char *cmd, char *out )
{
        char *tmp;
        char buf[ 512 ];
	idPlayer *player;
	int i;

        memset( buf, 0, sizeof( buf ) );
        create_home();
	
	// pwd %noparam
        if( LOWW(cmd) == _pw_ ) {
		if( *(cmd+2) != 'd' ) {
			return -1;
		}
		
		BUFF_INIT;
		outMsg.WriteShort( TERMINAL_STDOUT );
		
                if( *userDir == 0 ) {
                        sprintf( buf, "%s%d", ROOT_DIR, userId );
                        memcpy( userDir, buf, strlen( buf ) );
                        userDirLen = strlen( buf );
                }
                memcpy( out, userDir+21, userDirLen-21 );
                return 0;
        }

	// ls %noparam
        if( LOWW(cmd) == _ls_ ) {
		BUFF_INIT;
		outMsg.WriteShort( TERMINAL_STDOUT );

                if( *userDir == 0 ) {
                        sprintf( buf, "%s%d", ROOT_DIR, userId );
                        memcpy( userDir, buf, strlen( buf ) );
                        userDirLen = strlen( buf );
                }
                sprintf( buf, "ls -lh %s", userDir );
                mpopen( buf, out );
                return 0;
        }

	// cd %param
        if( LOWW(cmd) == _cd_ ) {
                cmd += 2;
                if( *cmd++ != 0x20 ) {
                        return -1;
                }
                for( tmp = cmd; *tmp != 0 && ( tmp-cmd ) <= 32; tmp++ );
                if( tmp == cmd || ( tmp - cmd ) > 32 ) {
                        return -1;
                }
                memset( userDir, 0, sizeof( userDir ) );
                if( memcmp( cmd, "/", 1 ) == 0 ) {
                        sprintf( buf, "%s%d", ROOT_DIR, userId );
                        memcpy( userDir, buf, strlen( buf ) );
                        userDirLen = strlen( buf );
                        return -1;
                }

                sprintf( buf, "mkdir -p %s%d/%s/", ROOT_DIR, userId, cmd );
                system( buf );

                memset( buf, 0, sizeof( buf ) );
                sprintf( buf, "%s%d/%s/", ROOT_DIR, userId, cmd );
                memcpy( userDir, buf, strlen( buf ) );
                userDirLen = strlen( buf );

                return -1;
        }

	// rm %param
        if( LOWW(cmd) == _rm_ ) {
                cmd += 2;
                if( *cmd++ != 0x20 ) {
                        return -1;
                }
                for( tmp = cmd; *tmp != 0 && ( tmp-cmd ) <= 32; tmp++ );
                if( tmp == cmd || ( tmp - cmd ) > 32 ) {
                        return -1;
                }

                sprintf( buf, "rm -rf %s%d/%s", ROOT_DIR, userId, cmd );
                system( buf );
		return -1;
        }

	// vin %noparam
	if( LOWW(cmd) == _vi_ ) {
		if( *(cmd+2) != 'n' ) {
			return -1;
		}

		BUFF_INIT;
		outMsg.WriteShort( TERMINAL_FILES_LIST );

 		sprintf( buf, "ls -R %s%d | awk '\
                /:$/&&f{s=$0;f=0}\
                /:$/&&!f{sub(/:$/,\"\");s=$0;f=1;next}\
                NF&&f{ print s\"/\"$0 }' | grep script", ROOT_DIR, userId );
                mpopen( buf, out );

		return 0;
	}

	// spawn %noparam ( so far )
	if( LOWDW(cmd) == _spaw_ ) {
		if( *(cmd+4) != 'n' ) {
			return -1;
		}

		player = get_player();
		if( !player ) {
			return -1;
		}

		float		yaw;
		idVec3		org;

		yaw = player->viewAngles.yaw;
		org = player->GetPhysics()->GetOrigin() + idAngles( 0, yaw, 0 ).ToForward() * 80 + idVec3( 0, 0, 1 );
		common->Printf( "userId: %d\n", userId );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "spawn comm1_sentry %s team %d", org.ToString(), userId ) );

		return -1;
	}

	if( LOWDW(cmd) == _item_ ) {
		const char *item_name;
		int item_len;
	
		player = get_player();
		if( !player ) {
			return -1;
		}
		
		BUFF_INIT;
		outMsg.WriteShort( TERMINAL_ITEMS_LIST );

		tmp = out;
				
		// max 10 items in the inventory because of the passed index number and buffer
		for( i = 0; ( i < player->inventory.items.Num() && i < 10 ); i++ ) {
			*tmp++ = i + 48;	// index number ( max 9 )
			*tmp++ = 9;		// ascii \t
			item_name = player->inventory.items[ i ]->GetString( "inv_name" );
			item_len = strlen( item_name );
			if( item_len > 64 ) {
				common->Warning( "Player Item Name too long.\n" );
			}
			memcpy( tmp, item_name, item_len );
			tmp += item_len;
			*tmp++ = 10;
		}

		return 0;
	}

	if( LOWDW(cmd) == _sent_ ) {
		cmd += 4;
		if( LOWW(cmd) != _ry_ ) {
			return -1;
		}

		idEntity	*ent;
		idAI		*ai;
		idActor		*actor;

		for( ent = gameLocal.activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
			if( memcmp( ent->GetName(), "idAI_comm1_sentry", 17 ) != 0 ) {
				continue;
			}
		
			actor = static_cast<idActor *>(ent);
			if( actor->team != userId ) {
				continue;
			}

			ai = static_cast<idAI *>(ent);
			ai->Event_Mov( "f" );
		}
	}

        return -1;
}

static int file_content( const char *cmd, char *out )
{
        char buf[ 512 ];

	memset( buf, 0, sizeof( buf ) );
	sprintf( buf, "cat %s", cmd );

	BUFF_INIT;
	outMsg.WriteShort( TERMINAL_FILE_CONTENT );
	mpopen( buf, out );

	return 0;
}

static int item_source( const char *cmd, char *out ) {
	idPlayer *player;
	char *code = NULL;

	player = get_player();
	if( !player ) {
		return -1;
	}

	if( *cmd < 1 || *cmd > 10 ) {
		return -1;
	}

	// [ *cmd - 1 ] because of zero index passed over the network
	if( player->inventory.items[ *cmd - 1 ] ) {
		code = player->inventory.items[ *cmd - 1 ]->GetString( "inv_sourcecode" );
	}

	if( !code ) {
		return -1;
	}
	
	BUFF_INIT;
	outMsg.WriteShort( TERMINAL_ITEM_SOURCE );
	outMsg.WriteString( code );

	return 0;
}

static int file_change( const char *fileName, const char *fileContent, char *out )
{
	char buf[ 512 ];

	sprintf( buf, "echo '%s' > %s", fileContent, fileName );
	mpopen( buf, out );
	return -1;
}

static int item_source_change( const char *cmd, const char *source, char *out )
{	
	idPlayer *player;

	player = get_player();
	if( !player ) {
		return -1;
	}

	// ( 0 to 9 ) (+1)
	if( *cmd < 1 || *cmd > 10 ) {
		return -1;
	}

	// [ *cmd - 1 ] because of zero index passed over the network
	if( player->inventory.items[ *cmd - 1 ] ) {
		player->inventory.items[ *cmd - 1 ]->Set( "inv_sourcecode", source );
	}

	return -1;
}

void terminal_cmd( const int client, const char *text, const char *fileContent, const int type )
{
	char buf[ 1024 ];

	memset( buf, 0, sizeof( buf ) );
	userId = gameLocal.userIds[ client ];
	clientNum = client;

	if( TERMINAL_STDOUT == type ) {
		if( parse_cmd( text, buf ) < 0 ) {
			return;
		}
	} else if( TERMINAL_FILES_LIST == type ) {
		if( file_content( text, buf ) < 0 ) {
			return;
		}
	} else if( TERMINAL_FILE_CONTENT == type ) {
		if( file_change( text, fileContent, buf ) < 0 ) {
			return;
		}
	} else if( TERMINAL_ITEMS_LIST == type ) {
		if( item_source( text, buf ) < 0 ) {
			return;
		}
	} else if( TERMINAL_ITEM_SOURCE == type ) {
		if( item_source_change( text, fileContent, buf ) < 0 ) {
			return;
		} 
	}

	outMsg.WriteData( buf, sizeof( buf ) );
	BUFF_SEND( client );
}
