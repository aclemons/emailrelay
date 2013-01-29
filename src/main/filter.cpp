//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ===
//
// filter.cpp
//
// Provides filter_run(), filter_main() and filter_help().
//

#include "gdef.h"
#include "garg.h"
#include "gfile.h"
#include "gstr.h"
#include "gexception.h"
#include "gdirectory.h"
#include "legal.h"
#include "filter.h"
#include <iostream>
#include <exception>
#include <stdexcept>
#include <sstream>
#include <list>
#include <string>

G_EXCEPTION( FilterError , "filter error" ) ;

void filter_help( const std::string & prefix )
{
	std::cout 
		<< "usage: " << prefix << " <emailrelay-content-file>" << std::endl
		<< std::endl
		<< "Copies the corresponding emailrelay envelope file into all " << std::endl
		<< "subdirectories of the spool directory. Exits with a " << std::endl
		<< "value of 100 if copied once or more. Intended for use " << std::endl
		<< "with \"emailrelay --pop-by-name\"." << std::endl
		<< std::endl
		<< Main::Legal::warranty(std::string(),"\n") << std::endl
		<< Main::Legal::copyright() << std::endl ;
}

bool filter_run( const std::string & content )
{
	// check the content file exists
	//
	G::Path content_path( content ) ;
	bool ok = G::File::exists( content_path ) ;
	if( !ok )
		throw FilterError( "no such file" ) ;

	// build the envelope name
	//
	G::Path dir_path = content_path.dirname() ;
	std::string envelope_name = content_path.basename() ;
	unsigned int n = G::Str::replaceAll( envelope_name , "content" , "envelope" ) ;
	if( n != 1U )
		throw FilterError( "invalid filename" ) ;

	// check the envelope file exists
	//
	G::Path envelope_path = G::Path( dir_path , envelope_name + ".new" ) ;
	if( ! G::File::exists(envelope_path) )
	{
		// fall back to no ".new" extension in case we are run manually for some reason
		G::Path envelope_path_alt = G::Path( dir_path , envelope_name ) ;
		if( G::File::exists(envelope_path_alt) )
			envelope_path = envelope_path_alt ;
		else
			throw FilterError( std::string() + "no envelope file \"" + envelope_path.str() + "\"" ) ;
	}

	// read the content "to" address
	std::string to = filter_read_to( content_path.str() ) ;

	// copy the envelope into all sub-directories
	//
	int directory_count = 0 ;
	std::list<std::string> failures ;
	G::Directory dir( dir_path ) ;
	G::DirectoryIterator iter( dir ) ;
	while( iter.more() && !iter.error() )
	{
		if( iter.isDir() && filter_match(iter.filePath(),to) )
		{
			directory_count++ ;
			G::Path target = G::Path( iter.filePath() , envelope_name ) ;
			bool copied = G::File::copy( envelope_path , target , G::File::NoThrow() ) ;
			if( !copied )
				failures.push_back( iter.fileName().str() ) ;
		}
	}

	// delete the parent envelope (ignore errors)
	//
	bool envelope_deleted = false ;
	if( directory_count > 0 && failures.empty() )
	{
		envelope_deleted = G::File::remove( envelope_path , G::File::NoThrow() ) ;
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
		throw FilterError( ss.str() ) ;
	}
	if( directory_count == 0 ) // probably a permissioning problem
	{
		throw FilterError( "no sub-directories to copy into: check permissions" ) ;
	}

	return envelope_deleted ;
}

int filter_main( int argc , char * argv [] )
{
	try
	{
		G::Arg args( argc , argv ) ;

		if( args.c() <= 1U )
			throw FilterError( "usage error: must be run by emailrelay with the full path of a message content file" ) ;

		if( args.v(1U) == "--help" )
		{
			filter_help( args.prefix() ) ;
			return 1 ;
		}
		else
		{
			return filter_run( args.v(1U) ) ? 100 : 0 ;
		}
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

/// \file filter.cpp
