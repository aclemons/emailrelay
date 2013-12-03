/*
   Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
	guistart.c

	A program that unpacks itself (if packed) and then runs
	"emailrelay-gui" (win32) or "emailrelay-gui.real" (*nix).

	The target program is searched for in various likely places.

	The first line of a "emailrelay-gui.cfg" file (if it exists)
	is put onto the target program's command-line. This is typically 
	only used during development to add debug switches etc.

	This is written in "C" so that a self-extracting archive does 
	not have any dependence on the C++ runtime library. This can
	be important for Windows since the C++ runtime may not be
	installed as standard.

	See also main/start.cpp.

*/

#include "gdef.h"
#include "unpack.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#define CONFIG_READ_BUFFER_SIZE 10000
#define ARGV_SIZE 100

#ifdef G_WIN32
static const char target_exe_name [] = "emailrelay-gui.exe" ;
#define slash "\\"
#define ARGV_CONST const
#else
static const char target_exe_name [] = "emailrelay-gui.real" ;
#define slash "/"
#define ARGV_CONST
#endif
static const char config_file [] = "emailrelay-gui.cfg" ;

static void split( char * buffer , ARGV_CONST char * argv [] ) ;
static int unpack( const char * path ) ;

static char * strdup_( const char * p_in )
{
	char * p_out = malloc( strlen(p_in) + 1 ) ;
	strcpy( p_out , p_in ) ;
	return p_out ;
}

static char * sanitise( const char * string_in )
{
	/* remove all shell meta characters, including quotes */
	char * result = strdup_( string_in ) ; /* leak */
	const char * cp = "$\\\"\'()[]<>|!~*?&;" ;
	for( ; *cp ; cp++ )
	{
		char * p_out = result ;
		char * p_in = result ;
		for( ; *p_in ; p_in++ )
		{
			if( *p_in != *cp )
				*p_out++ = *p_in ;
		}
		*p_out = '\0' ;
	}
	return result ;
}

static int match_end( const char * p1 , const char * p2 )
{
	const char * p = p2 ;
	if( strlen(p1) < strlen(p2) ) return 0 ;
	for( ; *p ; p++ )
	{
		if( tolower(*p) != tolower(*(p1+strlen(p1)-strlen(p))) )
			return 0 ;
	}
	return 1 ;
}

static char * join( const char * p1 , const char * p2 )
{
	char * dst = malloc( strlen(p1) + strlen(p2) + 10 ) ;
	strcpy( dst , p1 ) ;
	strcat( dst , p2 ) ;
	return dst ;
}

static const char * basename( const char * p )
{
	p = strrchr(p,'/') ? (strrchr(p,'/')+1) : p ;
	p = strrchr(p,'\\') ? (strrchr(p,'\\')+1) : p ;
	return strdup_( p ) ; /* leak */
}

static const char * dirname( const char * p_in )
{
	char * p = strdup_( p_in ) ; /* leak */
	char * p1 = strrchr( p , '/' ) ;
	char * p2 = strrchr( p , '\\' ) ;
	char * pp = p1 && p2 ? ( p1 > p2 ? p1 : p2 ) : ( p1 ? p1 : p2 ) ;
	if( pp ) 
	{
		*pp = '\0' ;
		return p ;
	}
	else
	{
		return "." ;
	}
}

#ifdef G_WIN32
static void on_error( const char * message , const char * more )
{
	char * buffer = malloc( strlen(message) + (more?strlen(more):0) + 10 ) ;
	sprintf( buffer , "%s%s%s\n" , message , (more&&*more?": ":"") , more?more:"" ) ;
	MessageBoxA( NULL , buffer , "error" , MB_OK ) ;
	exit( 1 ) ;
}
static void chmodx( const char * p )
{
	/* no-op */
}
static const char * add_dot_exe( const char * this_exe )
{
	if( ! match_end( this_exe , ".exe" ) )
	{
		this_exe = join( this_exe , ".exe" ) ;
	}
	return this_exe ;
}
#else
static void on_error( const char * message_in , const char * more )
{
	/* try running osascript for Mac OSX */
	{
		const char * message = sanitise( message_in ) ;
		const char * command_format = "/usr/bin/osascript -e \""
			"tell application \\\"AppleScript Runner\\\" to display dialog \\\"%s\\\" "
			"with title \\\"Error\\\" buttons {\\\"Cancel\\\"}"
			"\" 2>/dev/null" ;
		char * buffer = malloc( strlen(command_format) + strlen(message_in) + 10 ) ;
		sprintf( buffer , command_format , message ) ;
		system( buffer ) ;
	}

	fprintf( stderr , "startup error: %s%s%s\n" , message_in , (more&&*more?": ":"") , more?more:"" ) ;
	exit( 1 ) ;
}
static void chmodx( const char * p )
{
	int rc = chmod( p , S_IRUSR | S_IWUSR | S_IXUSR ) ;
	if( rc != 0 )
		on_error( "cannot chmod" , NULL ) ;
}
static const char * add_dot_exe( const char * this_exe )
{
	return this_exe ;
}
#endif

static void on_unpack_error( const char * message )
{
	on_error( message , NULL ) ;
}

static int packed_file( const char * path )
{
	Unpack * p = unpack_new( path , NULL ) ;
	if( p == NULL )
	{
		return 0 ;
	}
	else
	{
		unpack_delete( p ) ;
		return 1 ;
	}
}

static int unpack( const char * path )
{
	Unpack * p = unpack_new( path , on_unpack_error ) ;
	int ok = unpack_all( p , "." ) ;
	unpack_delete( p ) ;
	return ok ;
}

static int file_exists( const char * path )
{
	struct stat statbuf ;
	return 0 == stat( path , &statbuf ) && S_ISREG(statbuf.st_mode) ;
}

static char * find_target( const char * base_dir , const char * name )
{
	int levels = 4 ;
	int level = 0 ;
	int len = 0 ;
	char * buffer = malloc( strlen(base_dir) + strlen(name) + levels*3 + 10 ) ; /* leak */
	strcpy( buffer , base_dir ) ;
	strcat( buffer , slash ) ;
	for( ; level < levels ; level++ )
	{
		len = strlen(buffer) ;
		strcat( buffer , name ) ;
		if( file_exists(buffer) )
			return buffer ;
		buffer[len] = '\0' ;
		strcat( buffer , ".." slash ) ;
	}
	return NULL ;
}

int main( int argc_in , char * argv_in [] )
{
	int ok = 0 ;
	const char * this_exe = add_dot_exe( argv_in[0] ) ;
	const char * prefix = basename( argv_in[0] ) ;
	static char config_read_buffer[CONFIG_READ_BUFFER_SIZE+1] = { '\0' } ;
	char * target_exe = find_target( dirname(this_exe) , target_exe_name ) ;
	ARGV_CONST char * argv_out [ARGV_SIZE+1] = { NULL } ;
	int is_packed = packed_file( this_exe ) ;

	if( target_exe == NULL )
		on_error( "cannot find target program to run" , target_exe_name ) ;

	if( is_packed )
	{
		/* startup banner */
		printf( "%s: self-extracting archive for %s\n" , prefix , target_exe_name ) ;
		if( argc_in > 1 && argv_in[1][0] == '-' && ( argv_in[1][1] == 'h' || argv_in[1][1] == '-' ) )
		{
			printf( "  %s\n" , "http://emailrelay.sourceforge.net" ) ;
			return 0 ;
		}
		fflush( stdout ) ;

		/* unpack files */
		ok = unpack( this_exe ) ;
		if( !ok )
		{
			on_error( "unpack error" , this_exe ) ;
			return 1 ;
		}
	}

	/* read any extra command-line parameters from an optional config file */
	config_read_buffer[0] = '\0' ;
	{
		FILE * fp = fopen( config_file , "r" ) ;
		if( fp != NULL )
		{
			int rc = fread( config_read_buffer , 1 , sizeof(config_read_buffer)-1 , fp ) ;
			if( ferror(fp) )
				config_read_buffer[0] = '\0' ;
			config_read_buffer[rc] = '\0' ;
			config_read_buffer[sizeof(config_read_buffer)-1] = '\0' ;
			fclose( fp ) ;
		}
		if( strchr(config_read_buffer,'\n') )
			*strchr(config_read_buffer,'\n') = '\0' ;
	}

	/* run the target exe */
	{
		printf( "%s: running %s %s\n" , prefix , target_exe , config_read_buffer ) ;
		chmodx( target_exe ) ;
		split( config_read_buffer , argv_out ) ;
		argv_out[0] = target_exe ;
		execv( target_exe , argv_out ) ;
	}

	on_error( "exec error" , NULL ) ;
	return 1 ;
}

static void split( char * buffer , ARGV_CONST char * argv [] )
{
	int argc = 1 ;
	int in_quote = 0 ;
	int escaped = 0 ;
	char * in = buffer ;
	char * out = buffer ;
	char * current = NULL ;
	for( ; *in ; in++ )
	{
		if( *in == '\\' )
		{
			escaped = 1 ;
		}
		else
		{
			if( *in == '"' && !escaped )
			{
				in_quote = !in_quote ;
			}
			else if( *in == ' ' && !in_quote )
			{
				int repeat = out != buffer && *(out-1) == '\0' ;
				if( ! repeat )
				{
					*out++ = '\0' ;
					if( current != NULL && argc < ARGV_SIZE )
						argv[argc++] = current ;
					current = NULL ;
				}
			}
			else
			{
				if( current == NULL )
					current = out ;
				*out++ = *in ;
			}
			escaped = 0 ;
		}
	}
	*out = '\0' ;
	if( current != NULL && argc < ARGV_SIZE )
		argv[argc++] = current ;
	argv[argc] = NULL ;
}

