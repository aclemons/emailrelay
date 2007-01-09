/*
   Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
   
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later
   version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
   
*/

/*
	run.c

	A program that unpacks itself and then execs "./emailrelay-gui".

	The first line of any "emailrelay-gui.cfg" file is put onto the
	command-line.

	This is written in "C" so that is does no have to be dependent
	on non-standard (ie. mingw) DLLs when built on windows.
*/

#include "gdef.h"
#include "unpack.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
static char exe [] = "emailrelay-gui.exe" ;
#else
static char exe [] = "emailrelay-gui" ;
#endif
static char cfg [] = "emailrelay-gui.cfg" ;

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
#endif

#define BUFFER_SIZE 10000
#define ARGV_SIZE 100

int main( int argc_in , char * argv_in [] )
{
	static char buffer[BUFFER_SIZE+1] ;
	char * argv [ARGV_SIZE+1] = { exe , NULL } ;

	/* unpack files */
	int ok = unpack( argv_in[0] ) ;
	if( !ok ) 
	{
		on_error( "unpack error" ) ;
		return 1 ;
	}

	/* read any extra command-line parameters */
	{
		FILE * fp = fopen( cfg , "r" ) ;
		buffer[0] = '\0' ;
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

	/* split up the command-line */
	split( buffer , argv ) ;

	/* run the target exe */
	{
		chmodx( exe ) ;
		execv( exe , argv ) ;
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

