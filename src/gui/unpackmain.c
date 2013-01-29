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
	unpackmain.c

	A utility that unpacks the given packed file.
	If no file is specified then it tries argv[0]
	(ie. unpacking from itself) and then "payload".

*/

#include "gdef.h"
#include "unpack.h"
#include <string.h>
#include <stdio.h>

static void on_error( const char * message )
{
	message = message ;
}

static int unpack( const char * path )
{
	Unpack * p = unpack_new( path , on_error ) ;
	int ok = unpack_all( p , "." ) ;
	unpack_delete( p ) ;
	return ok ;
}

int main( int argc , char * argv [] )
{
	int ok = 0 ;
	const char * prefix = argv[0] ;
	const char * path = argc > 1 ? argv[1] : argv[0] ;

	prefix = strrchr(prefix,'/') ? (strrchr(prefix,'/')+1) : prefix ;
	prefix = strrchr(prefix,'\\') ? (strrchr(prefix,'\\')+1) : prefix ;

	ok = unpack( path ) ;
	if( !ok )
		fprintf( stderr , "%s: failed to unpack %s\n" , prefix , path ) ;

	if( argc == 1 && !ok )
	{
		path = "payload" ;
		ok = unpack( "payload" ) ;
		if( !ok )
			fprintf( stderr , "%s: failed to unpack %s\n" , prefix , path ) ;
	}

	return ok ? 0 : 1 ;
}

