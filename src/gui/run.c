/*
   Copyright (C) 2001-2008 Graeme Walker <graeme_walker@users.sourceforge.net>
   
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
	run.c

	A program that unpacks itself and then execs "./emailrelay-gui".

	The first line of any "emailrelay-gui.cfg" file is put onto the
	command-line. This is typically only used during development to add
	debug switches etc.

	This is written in "C" so that a self-extracting archive does 
	not have any dependence on the C++ runtime library. This is 
	important for Windows since the MinGW C++ runtime is not installed
	as standard.

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

#ifdef _WIN32
static char gui_exe [] = "emailrelay-gui.exe" ;
#else
static char gui_exe [] = "emailrelay-gui.real" ;
#endif
static char gui_cfg [] = "emailrelay-gui.cfg" ;

static void split( char * buffer , char * argv [] ) ;
static int unpack( const char * path ) ;

#ifdef _WIN32
#include <windows.h>
static void on_error( const char * p )
{
	MessageBox( NULL , p , "error" , MB_OK ) ;
        exit( 1 ) ;
}
static void chmodx( const char * p )
{
	/* no-op */
}
static char * strcat_( const char * p1 , const char * p2 )
{
	char * dst = malloc(strlen(p1)+strlen(p2)+10) ;
	static char nothing[] = { '\0' } ;
	if( dst == NULL ) return nothing ;
	strcpy( dst , p1 ) ;
	strcat( dst , p2 ) ;
	return dst ;
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
static const char * add_dot_exe( const char * this_exe )
{
	if( ! match_end( this_exe , ".exe" ) )
	{
		this_exe = strcat_( this_exe , ".exe" ) ;
	}
	return this_exe ;
}
#else
#include <stdio.h>
#include <unistd.h>
static void on_error( const char * p )
{
	fprintf( stderr , "unpack error: %s\n" , p ) ;
	exit( 1 ) ;
}
static void chmodx( const char * p )
{
	int rc = chmod( p , S_IRUSR | S_IWUSR | S_IXUSR ) ;
	if( rc != 0 )
		on_error( "cannot chmod" ) ;
}
static const char * add_dot_exe( const char * this_exe )
{
	return this_exe ;
}
#endif

#define BUFFER_SIZE 10000
#define ARGV_SIZE 100

int main( int argc_in , char * argv_in [] )
{
	int ok = 0 ;
	const char * this_exe = argv_in[0] ;
	static char buffer[BUFFER_SIZE+1] = { '\0' } ;
	char * argv [ARGV_SIZE+1] = { gui_exe , NULL } ;
	const char * prefix = argv_in[0] ;
	prefix = strrchr(prefix,'/') ? (strrchr(prefix,'/')+1) : prefix ;
	prefix = strrchr(prefix,'\\') ? (strrchr(prefix,'\\')+1) : prefix ;

	/* startup banner */
	printf( "%s: self-extracting archive for %s\n" , prefix , argv[0] ) ;
	if( argc_in > 1 && argv_in[1][0] == '-' && ( argv_in[1][1] == 'h' || argv_in[1][1] == '-' ) )
	{
		printf( "  %s\n" , "http://emailrelay.sourceforge.net" ) ;
		return 0 ;
	}
	fflush( stdout ) ;

	/* unpack files */
	ok = unpack( add_dot_exe(this_exe) ) ;
	if( !ok )
	{
		on_error( "unpack error" ) ;
		return 1 ;
	}

	/* read any extra command-line parameters from an optional config file */
	buffer[0] = '\0' ;
	{
		FILE * fp = fopen( gui_cfg , "r" ) ;
		if( fp != NULL )
		{
			int rc = fread( buffer , 1 , BUFFER_SIZE , fp ) ;
			if( ferror(fp) )
				buffer[0] = '\0' ;
			buffer[rc] = '\0' ;
			buffer[BUFFER_SIZE] = '\0' ;
			fclose( fp ) ;
		}
		if( strchr(buffer,'\n') )
			*strchr(buffer,'\n') = '\0' ;
	}
	printf( "%s: running %s %s\n" , prefix , gui_exe , buffer ) ;

	/* split up the command-line */
	split( buffer , argv ) ;

	/* run the target exe */
	{
		chmodx( gui_exe ) ;
		execv( gui_exe , argv ) ;
	}

	on_error( "exec error" ) ;
	return 1 ;
}

static void split( char * buffer , char * argv [] )
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

static int unpack( const char * path )
{
	Unpack * p = unpack_new( path , on_error ) ;
	int ok = unpack_all( p , "." ) ;
	unpack_delete( p ) ;
	return ok ;
}

