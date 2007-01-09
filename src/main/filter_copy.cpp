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
// to copy the message envelope into all spool sub-directories 
// for use by "--pop-by-name".
//
// The envelope in the parent directory is removed iff it
// has been copied at least once.
// 
// Note that the pop-by-name feature has the pop server look 
// for envelopes in a user-specific sub-directory and look for
// content files in the parent directory. If the envelope
// is deleted in the sub-directory and the content file is
// found to have no corresponding envelopes in any other
// sub-directory then the content is deleted too.
// See GPop::StoreLock::unlinked().
//
//


#include "garg.h"
#include "gfile.h"
#include "gstr.h"
#include "gexception.h"
#include "gdirectory.h"
#include <iostream>
#include <exception>
#include <stdexcept>
#include <sstream>
#include <list>
#include <string>

G_EXCEPTION( Error , "message copy" ) ;

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
	G::Path envelope_path = dir_path ; envelope_path.pathAppend(envelope_name+".new") ;
	if( ! G::File::exists(envelope_path) )
		throw Error( std::string() + "no envelope file \"" + envelope_path.str() + "\"" ) ;

	// copy the envelope into all sub-directories
	//
	int directory_count = 0 ;
	std::list<std::string> failures ;
	G::Directory dir( dir_path ) ;
	G::DirectoryIterator iter( dir ) ;
	while( iter.more() && !iter.error() )
	{
		if( iter.isDir() )
		{
			directory_count++ ;
			G::Path target = iter.filePath() ; target.pathAppend(envelope_name) ;
			bool ok = G::File::copy( envelope_path , target , G::File::NoThrow() ) ;
			if( !ok )
				failures.push_back( iter.fileName().str() ) ;
		}
	}

	// delete the parent envelope (ignore errors)
	//
	if( directory_count > 0 && failures.empty() )
	{
		G::File::remove( envelope_path , G::File::NoThrow() ) ;
	}

	// notify failures
	//
	if( ! failures.empty() )
	{
		std::ostringstream ss ;
		ss << "failed to copy envelope file " << envelope_path.str() << " into " ;
		if( failures.size() == 1U )
		{
			ss << "the \"" << failures.front() << "\" sub-directory" ;
		}
		else
		{
			ss << failures.size() << " sub-directories, including \"" << failures.front() << "\"" ;
		}
		throw Error( ss.str() ) ;
	}
	if( directory_count == 0 ) // probably a permissioning problem
	{
		throw Error( "no sub-directories to copy into" ) ;
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

