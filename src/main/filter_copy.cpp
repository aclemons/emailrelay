//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// filter_copy.cpp
//
// A utility that can be installed as a "--filter" program 
// to copy the message envelope into all spool subdirectories 
// for use by "--pop-by-name".
//


#include "garg.h"
#include "gfile.h"
#include "gstr.h"
#include "gexception.h"
#include "gdirectory.h"
#include <iostream>
#include <exception>
#include <stdexcept>

G_EXCEPTION( Error , "filter_copy" ) ;

static void run( const std::string & content )
{
	// check the content file exists
	//
	G::Path content_path(content) ;
	bool ok = G::File::exists(content_path) ;
	if( !ok )
		throw Error( "no such file" ) ;

	// build the envelope name
	//
	G::Path dir_path = content_path.dirname() ;
	std::string envelope_name = content_path.basename() ;
	unsigned int n = G::Str::replaceAll( envelope_name , "content" , "envelope" ) ;
	if( n != 1U )
		throw Error( "invalid filename" ) ;

	// check the envelope file exists
	//
	G::Path envelope_path = dir_path ; envelope_path.pathAppend(envelope_name) ;
	if( ! G::File::exists(envelope_path) )
		throw Error( "no envelope file" ) ;

	// copy the envelope into all subdirectories
	//
	G::Directory dir( dir_path ) ;
	G::DirectoryIterator iter( dir ) ;
	while( iter.more() && !iter.error() )
	{
		if( iter.isDir() )
		{
			G::Path target = iter.filePath() ; target.pathAppend(envelope_name) ;
			std::cout << envelope_path << " -> " << target << std::endl ;
			G::File::copy( envelope_path , target ) ;
		}
	}
}

int main( int argc , char * argv [] )
{
	try
	{
		G::Arg args( argc , argv ) ;
		if( args.c() <= 1U )
			throw Error( "usage error" ) ;

		run( args.v(1U) ) ;
		return 0 ;
	}
	catch( std::exception & e )
	{
		std::cout << "<<" << e.what() << ">>" << std::endl ;
	}
	catch(...)
	{
		std::cout << "<<" << "exception" << ">>" << std::endl ;
	}
	return 1 ;
}

