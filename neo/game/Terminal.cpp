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
	TERMINAL_FILE_CONTENT
};

static idBitMsg outMsg;
static byte msgBuf[ MAX_GAME_MESSAGE_SIZE ];
static int userId;
static char userDir[ 512 ];
static int userDirLen;

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
	/*
	len = strlen( cmd );
        memcpy( tmp, cmd, len );
	tmp += len;
	*tmp++ = 0x3a;
	*/
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
                for( tmp = cmd; *tmp != 0; tmp++ );
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
                for( tmp = cmd; *tmp != 0; tmp++ );
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

static int file_change( const char *fileName, const char *fileContent, char *out )
{
	char buf[ 4096 ];

	sprintf( buf, "echo '%s' > %s", fileContent, fileName );
	mpopen( buf, out );
	return -1;
}

void terminal_cmd( const int client, const char *text, const char *fileContent, const int type )
{
	char buf[ 1024 ];

	memset( buf, 0, sizeof( buf ) );
	userId = gameLocal.userIds[ client ];

	if( 0 == type ) {
		if( parse_cmd( text, buf ) < 0 ) {
			return;
		}
	} else if( 1 == type ) {
		if( file_content( text, buf ) < 0 ) {
			return;
		}
	} else if( 2 == type ) {
		if( file_change( text, fileContent, buf ) < 0 ) {
			return;
		}
	}

	common->Printf( "[%s] tt\n", buf );

	outMsg.WriteData( buf, sizeof( buf ) );
	BUFF_SEND( client );
}
