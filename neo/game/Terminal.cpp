#include "../idlib/precompiled.h"
#pragma hdrstop

#define ROOT_DIR        "/home/brick/.fmfusers/"
#define ROOT_DIR_LEN    22

static idBitMsg outMsg;
static byte msgBuf[ MAX_GAME_MESSAGE_SIZE ];
static int userId;
static char userDir[ 512 ];
static int userDirLen;

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

static void mpopen( const char *cmd, char *out )
{
  	FILE *fp;
  	char path[ 8192 ];
        int len;
        char *tmp;
	
	memset( path, 0, sizeof( path ) );

	fp = popen( cmd , "r" );
  	if( fp == NULL ) {
    		common->Printf( "Failed to run command\n" );
    		return;
  	}

        tmp = out;
	len = strlen( cmd );
        memcpy( tmp, cmd, len );
	tmp += len;
	*tmp++ = 0x3a;
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

static int parse_cmd( char *cmd, char *out )
{
        char *tmp;
        char buf[ 512 ];

        memset( buf, 0, sizeof( buf ) );
        create_home();
        if( memcmp( cmd, "pwd", 3 ) == 0 ) {
                if( *userDir == 0 ) {
                        sprintf( buf, "%s%d", ROOT_DIR, userId );
                        memcpy( userDir, buf, strlen( buf ) );
                        userDirLen = strlen( buf );
                }
                memcpy( out, userDir+21, userDirLen-21 );
                return 0;
        }

        if( memcmp( cmd, "ls", 2 ) == 0 ) {
                if( *userDir == 0 ) {
                        sprintf( buf, "%s%d", ROOT_DIR, userId );
                        memcpy( userDir, buf, strlen( buf ) );
                        userDirLen = strlen( buf );
                }
                sprintf( buf, "ls -lh %s", userDir );
                mpopen( buf, out );
                return 0;
        }

        if( memcmp( cmd, "cd", 2 ) == 0 ) {
                cmd += 2;
                if( *cmd++ != 0x20 ) {
                        return -1;
                }
                for( tmp = cmd; *tmp != 0; tmp++ );
                if( tmp == cmd || ( tmp - cmd ) > 32 ) {
                        return -1;
                }
                memset( userDir, 0, sizeof( userDir ) );
                // home dir
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

        if( memcmp( cmd, "rm", 2 ) == 0 ) {
                cmd += 2;
                if( *cmd++ != 0x20 ) {
                        return -1;
                }
                for( tmp = cmd; *tmp != 0; tmp++ );
                if( tmp == cmd || ( tmp - cmd ) > 32 ) {
                        return -1;
                }

                sprintf( buf, "rm -rf %s%d/%s", ROOT_DIR, userId, cmd );
                system( buf );
        }

        return -1;
}

void terminal_cmd( const int client, const char *text )
{
	char buf[ 1024 ];

	memset( buf, 0, sizeof( buf ) );
	userId = gameLocal.userIds[ client ];

	if( parse_cmd( text, buf ) < 0 ) {
		return;
	}

	common->Printf( "[%s]\n", buf );

	BUFF_INIT;
	outMsg.WriteData( buf, sizeof( buf ) );
	BUFF_SEND( client );
}
