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
// guistart.cpp
//
// A simple wrapper for Mac OS X that runs the "emailrelay-gui.real"
// binary. Searches for the executable in various likely locations
// relative to argv0. Errors are reported using an osascript dialog
// box.
//
// See also main/start.cpp.
//

#include "gdef.h"
#include <iostream>
#include <exception>
#include <stdexcept>
#include <string>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <functional>
#include <list>
#include <sys/types.h>
#include <sys/stat.h>

static bool exists( std::string path )
{
	struct stat statbuf ;
	return 0 == ::stat( path.c_str() , &statbuf ) && S_ISREG(statbuf.st_mode) ;
}

static std::string dirname( std::string path )
{
	std::string::size_type pos = path.find_last_of( "/\\" ) ;
	std::string base = pos == std::string::npos ? "." : path.substr(0U,pos) ;
	return base ;
}

static std::string find( std::string base , std::string name )
{
	std::list<std::string> list ;
	list.push_back( base + "/" + name ) ;
	list.push_back( base + "/../" + name ) ;
	list.push_back( base + "/../../" + name ) ;
	list.push_back( base + "/../../../" + name ) ;
	list.push_back( base + "/../../../../" + name ) ;
	for( std::list<std::string>::iterator p = list.begin() ; p != list.end() ; ++p )
	{
		if( exists(*p) )
		{
			std::cout << "found [" << *p << "]" << std::endl ;
			return *p ;
		}
	}
	std::cout << "not found ...\n " ;
	std::copy( list.begin() , list.end() , std::ostream_iterator<std::string>(std::cout,"\n ") ) ;
	std::cout << "\n" ;
	return std::string() ;
}

static void exec( std::string exe )
{
	char * argv[] = { const_cast<char*>(exe.c_str()) , NULL } ;
	::execv( exe.c_str() , argv ) ;
}

static void remove( std::string & s , char c )
{
	s.erase( std::remove_if( s.begin() , s.end() , std::bind1st(std::equal_to<char>(),c) ) , s.end() ) ;
}

static void sanitise( std::string & s )
{
	// remove all shell meta characters, including quotes
	for( const char * p = "$\\\"\'()[]<>|!~*?&;" ; *p ; p++ )
		remove( s , *p ) ;
}

static std::string sanitised( std::string s )
{
	sanitise( s ) ;
	return s ;
}

int main( int , char * argv [] )
{
	try
	{
		std::string exe = find( dirname(argv[0]) , "emailrelay-gui.real" ) ;
		if( exe.empty() )
			throw std::runtime_error( std::string() + "no executable" ) ;

		exec( exe ) ;
		throw std::runtime_error( std::string() + "cannot exec [" + exe + "]" ) ;
	}
	catch( std::exception & e )
	{
		std::cerr << "exception: " << e.what() << std::endl ;

		// dialog box for mac
		std::stringstream ss ;
		ss
			<< "/usr/bin/osascript -e \""
				<< "display dialog \\\"" << sanitised(e.what()) << "\\\" "
				<< "with title \\\"Error\\\" buttons {\\\"Cancel\\\"}"
			<< "\" 2>/dev/null" ;
		::system( ss.str().c_str() ) ;
	}
	return 1 ;
}

/// \file guistart.cpp
