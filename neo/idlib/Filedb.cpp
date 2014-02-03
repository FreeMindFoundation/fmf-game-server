#include "../idlib/precompiled.h"
#pragma hdrstop

static long lSize;
static pthread_mutex_t filemutex = PTHREAD_MUTEX_INITIALIZER;
#define FILE_DB		"~/.fmf_dusers"

#define ERR_RET( msg )\
	pthread_mutex_unlock( &filemutex );\
	printf( "%s\n", msg );\
	return NULL;

static inline char *readFile()
{
        FILE *file;
        char *buffer;
	size_t read;

	pthread_mutex_lock( &filemutex );

	file = fopen( FILE_DB, "r" );
	if( file == NULL ) {
		ERR_RET( "fopen error" );
	}

	fseek( file, 0, SEEK_END );
	lSize = ftell( file );
	rewind( file );
	buffer = (char*) malloc( sizeof(char) * lSize );
	if( buffer == NULL ) {
		ERR_RET( "malloc error" );
	}
	read = fread( buffer, 1, lSize, file );
	if( lSize != read ) {
		ERR_RET( "size error" );
	}
	fclose( file );

	pthread_mutex_unlock( &filemutex );

	return buffer;
}

static inline void updateValue( const int *pos, void *value, const int len )
{
 	FILE *file;
	char buffer[ len ];
	memcpy( buffer, value, len );
	pthread_mutex_lock( &filemutex );
	file = fopen( FILE_DB, "rb+" );
	if( file == NULL ) {
		pthread_mutex_unlock( &filemutex );
		printf( "fopen error\n" );
		return;
	}
	fseek( file, *pos, SEEK_SET );
	fwrite( buffer, 1, sizeof( buffer),  file );
  	fclose( file );
	pthread_mutex_unlock( &filemutex );
}

static inline void addUser( User_t *u ) 
{
  	FILE *file;
	if( u == NULL ) {
		return;
	}
	pthread_mutex_lock( &filemutex );
  	file = fopen( FILE_DB , "a" );
	if( file == NULL ) {
		pthread_mutex_unlock( &filemutex );
		printf( "fopen error\n" );
		return;
	}
  	fwrite( (char *)u , sizeof( User_t ), 1, file );
  	fclose( file );
	pthread_mutex_unlock( &filemutex );
}

static inline char userExists( const char *username ) 
{
	char *buffer;
	char exists;
	User_t *u;
	int i;

	exists = 0;
	buffer = readFile();

	if( buffer == NULL ) {
		return exists;	
	}
	
	u = (User_t *)buffer;
	for( i = 0; i < lSize; i += sizeof( User_t ) ) {
		if( memcmp( u->username + i, username, strlen( username ) ) == 0 ) {
			exists = 1;
		}
	}

	free( buffer );
	return exists;	
}

static inline int getNextID()
{
	char *buffer;
	buffer = readFile();
	int userId = 0;	
	User_t *u;

	if( buffer == NULL ) {
		return userId;
	}	

	u = (User_t *)( buffer + lSize - sizeof( User_t ) );
	userId = u->userId + 1;
	free( buffer );
	return userId;
}

void db_hashPass( const char *pass, unsigned char *buffer )
{
	SHA256_CTX ctx;

	sha256_init( &ctx );
   	sha256_update( &ctx, (unsigned char *)pass, strlen( (const char *)pass ) );
   	sha256_final( &ctx, buffer );
}

char db_createUser( const char *username, const char *password )
{
	if( strlen( username ) > 32 || strlen( password ) > 32 )
		return DB_ERR_CREATE_LENGTH;
	
	if( userExists( username ) )
		return DB_ERR_CREATE_EXISTS;

   	unsigned char hashpass[ 32 ];
	int userId = 0;	
	User_t *user;

	db_hashPass( password, hashpass );

	userId = getNextID();
	user = malloc( sizeof( User_t ) );
	memset( user, 0, sizeof( User_t ) );
	user->userId = userId;
	memcpy( user->username, username, strlen( username ) );
	memcpy( user->userpass, hashpass, sizeof( hashpass ) );
	user->flags = USER_ISACTIVE;
	addUser( user );
	free( user );
		
	return DB_CREATE;
}

static inline int getPosByUser( const char *username, char *buffer )
{
	User_t *u;
	u = (User_t *)buffer;

	for( int i = 0; i < lSize; i += sizeof( User_t ) ) {
		if( strcmp( u->username + i, username ) == 0 ) {
			return i;
		}
	}
	
	return -1;
}

int db_getUserFlag( const char *username, const int flag )
{
	if( strlen( username ) < 1 )
		return 0;

	int pos;
	User_t *u;
	char *buffer;
	int rFlag = -1;

	buffer = readFile();
	pos = getPosByUser( username, buffer );	
	if( pos == -1 ) {
		printf("user not found\n");
		return rFlag;
	}
	u = (User_t *)( buffer + pos );
	rFlag = u->flags & flag;

	free( buffer );
	return rFlag;
}

void db_setUserFlag( const char *username, const int value )
{
	if( strlen( username ) < 1 )
		return;

	int pos, tmp;
	User_t u, *pu;
	char *buffer;

	buffer = readFile();
	pos = getPosByUser( username, buffer );
	pu = (User_t *)( buffer + pos );
	tmp = value | pu->flags;
	pos += (int )&u.flags - (int )&u;
	updateValue( &pos, (void *)&tmp, sizeof( int ) );

	free( buffer );
}

static inline void print_hash( const unsigned char* c )
{
	int i;

	for( i = 0; i < 32; i++ )
        	printf( "%X", *c++ );

	printf( "\n" );
}

char db_verifyUser( const char *username, unsigned char userpass[] )
{
	int pos;
	User_t *u;
	char *buffer;
	char r;	

	r = -1;

	if( strlen( username ) < 1 )
		return -1;

	buffer = readFile();
	if( buffer == NULL )
		return r;
	
	pos = getPosByUser( username, buffer );	
	u = (User_t *)( buffer + pos );
	
	if( memcmp( u->userpass, userpass, SHA256_LEN ) == 0 ) {
		r = 0;
	}	

	free( buffer );
	
	return r;
}

char db_verifyUserHash( const char *username, const char *userpass )
{
	unsigned char hash[32];

	db_hashPass( userpass, hash );
	return ( db_verifyUser( username, hash ) );
}
