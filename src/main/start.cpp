//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
///
/// \file start.cpp
///
// A simple wrapper that runs the main emailrelay binary with a
// command-line assembled from the main configuration file (as used
// by the init.d startup script). Always adds "--as-server".
//
// The motivation for this is that a c/c++ program is easier to put
// into a Mac bundle than a shell script.
//
// Searches for the executable and the configuration file in various
// likely locations relative to argv0.
//

#include "gdef.h"
#include <iostream>
#include <exception>
#include <stdexcept>
#include <string>
#include <fstream>
#include <sstream>
#include <ostream>
#include <iterator>
#include <algorithm>
#include <functional>
#include <list>
#include <cstdlib> // system()
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <cerrno>

static bool exists( std::string path )
{
	struct stat statbuf ;
	return 0 == ::stat( path.c_str() , &statbuf ) && S_ISREG(statbuf.st_mode) ;
}

static std::list<std::string> simple_candidates( std::string base , std::string name )
{
	std::list<std::string> list ;
	list.push_back( base + "/" + name ) ;
	list.push_back( base + "/../" + name ) ;
	list.push_back( base + "/../../" + name ) ;
	list.push_back( base + "/../../../" + name ) ;
	list.push_back( base + "/../../../../" + name ) ;
	return list ;
}

static std::list<std::string> config_candidates( std::string base , std::string name )
{
	std::list<std::string> list = simple_candidates( base , name ) ;
	list.push_back( base + "/../Resources/" + name ) ;
	list.push_back( base + "/../../etc/" + name ) ;
	list.push_back( base + "/../../Library/Preferences/E-MailRelay/" + name ) ;
	list.push_back( base + "/../../../etc/" + name ) ;
	list.push_back( base + "/../../../Library/Preferences/E-MailRelay/" + name ) ;
	list.push_back( base + "/../../../../etc/" + name ) ;
	list.push_back( base + "/../../../../Library/Preferences/E-MailRelay/" + name ) ;
	list.push_back( base + "/../../../../../etc/" + name ) ;
	list.push_back( base + "/../../../../../Library/Preferences/E-MailRelay/" + name ) ;
	return list ;
}

static std::list<std::string> gui_candidates( std::string base )
{
	std::list<std::string> list ;
	list.push_back( base + "/E-MailRelay-Configure.app/Contents/MacOS/E-MailRelay-Configure" ) ;
	list.push_back( base + "/../../../E-MailRelay-Configure.app/Contents/MacOS/E-MailRelay-Configure" ) ;
	list.push_back( base + "/emailrelay-gui.real" ) ;
	list.push_back( base + "/../../../emailrelay-gui.real" ) ;
	return list ;
}

static std::string find( std::list<std::string> list )
{
	for( const auto & path : list )
	{
		if( exists(path) )
		{
			std::cout << "found [" << path << "]" << std::endl ;
			return path ;
		}
	}
	std::cout << "not found ...\n " ;
	std::copy( list.begin() , list.end() , std::ostream_iterator<std::string>(std::cout,"\n ") ) ;
	std::cout << "\n" ;
	return std::string() ;
}

static void rtrim( std::string & s )
{
	std::string::size_type n = s.find_last_not_of( " \t" ) ;
	if( n == std::string::npos )
		s = std::string() ;
	else if( (n+1U) != s.length() )
		s.resize( n + 1U ) ;
}

static void ltrim( std::string & s )
{
	std::string::size_type n = s.find_first_not_of( " \t" ) ;
	if( n == std::string::npos )
		s = std::string() ;
	else if( n != 0U )
		s.erase( 0U , n ) ;
}

static void trim( std::string & s )
{
	ltrim( s ) ;
	rtrim( s ) ;
}

static void remove( std::string & s , char c )
{
	s.erase( std::remove_if( s.begin() , s.end() , [=](char x){return c == x;} ) , s.end() ) ;
}

static void sanitise( std::string & s )
{
	// remove all shell meta characters
	for( const char * p = "$\\\"\'()[]<>|!~*?&;" ; *p ; p++ )
		remove( s , *p ) ;
}

static std::string sanitised( std::string s )
{
	sanitise( s ) ;
	return s ;
}

static std::list<std::string> read( std::string path )
{
	std::list<std::string> result ;
	result.push_back( "--as-server" ) ;

	std::ifstream s( path.c_str() ) ;
	while( s.good() )
	{
		std::string line ;
		std::getline( s , line ) ;
		trim( line ) ;
		if( !line.empty() && line.at(0U) != '#' && line.find("gui-") != 0U )
		{
			// change "--foo bar" to "--foo=bar"
			std::string::size_type npos = std::string::npos ;
			std::string::size_type sp = line.find(' ') ;
			std::string::size_type eq = line.find('=') ;
			std::string::size_type qq = line.find('\"') ;
			if( sp != npos && ( qq == npos || sp < qq ) && ( eq == npos && sp < eq ) )
				line.replace( sp , 1U , "=" ) ;

			result.push_back( std::string() + "--" + line ) ;
		}
	}
	return result ;
}

static void exec( std::string exe , std::list<std::string> args )
{
	char ** argv = new char* [args.size()+2U] ;
	int i = 1 ;
	for( auto p = args.cbegin() ; p != args.cend() ; ++p , i++ )
	{
		argv[i] = const_cast<char*>((*p).c_str()) ;
	}
	argv[0] = const_cast<char*>(exe.c_str()) ;
	argv[i] = nullptr ;
	::execv( exe.c_str() , argv ) ;
}

static void run( std::string exe , std::list<std::string> args )
{
	int fds[2] = { -1 , -1 } ;
	int rc = ::pipe( fds ) ;
	if( rc < 0 )
		throw std::runtime_error( "cannot create a pipe" ) ;

	rc = fork() ;
	if( rc == -1 )
	{
		throw std::runtime_error( "cannot fork" ) ;
	}
	else if( rc == 0 )
	{
		::close( fds[0] ) ;
		int fdout = fds[1] ;
		::close( STDERR_FILENO ) ;
		if( ::dup2(fdout,STDERR_FILENO) != STDERR_FILENO )
			throw std::runtime_error( "cannot dup" ) ;
		::close( fdout ) ; // since dup()ed
		::fcntl( STDERR_FILENO , F_SETFD , 0 ) ; // no-close-on-exec
		exec( exe , args ) ;
		throw std::runtime_error( "cannot exec" ) ;
	}
	else
	{
		::close( fds[1] ) ;
		int fdin = fds[0] ;
		int status = 0 ;
		for(;;)
		{
			rc = ::waitpid( rc , &status , 0 ) ;
			if( rc == -1 && errno != EINTR )
				throw std::runtime_error( "wait error" ) ;
			else if( rc != -1 )
				break ;
		}
		int exit_status = WEXITSTATUS(status) ;
		if( exit_status != 0 )
		{
			char buffer[1000] = { '\0' } ;
			rc = ::read( fdin , buffer , sizeof(buffer) ) ;
			if( rc < 0 )
				throw std::runtime_error( "cannot read from pipe" ) ;
			buffer[sizeof(buffer)-1] = '\0' ;
			std::string reason( buffer , rc ) ;
			remove( reason , '\n' ) ;
			throw std::runtime_error( reason ) ;
		}
	}
}

std::string join( const std::list<std::string> & list )
{
	std::ostringstream ss ;
	const char * sep = "" ;
	for( auto p = list.begin() ; p != list.end() ; ++p , sep = " " )
	{
		ss << sep << *p ;
	}
	return ss.str() ;
}

int main( int , char * argv [] )
{
	std::string gui ;
	try
	{
		std::string argv0( argv[0] ) ;
		std::string::size_type pos = argv0.find_last_of( "/\\" ) ;
		std::string base = pos == std::string::npos ? "." : argv0.substr(0U,pos) ;

		std::string cfg = find( config_candidates(base,"emailrelay.conf") ) ;
		if( cfg.empty() )
			throw std::runtime_error( std::string() + "no config file" ) ;

		std::string exe = find( simple_candidates(base,"emailrelay") ) ;
		if( exe.empty() )
			throw std::runtime_error( std::string() + "no executable" ) ;

		gui = find( gui_candidates(base) ) ;

		std::list<std::string> args = read( cfg ) ;
		std::cout << "exe [" << exe << "]" << std::endl ;
		std::cout << "args [" << join(args) << "]" << std::endl ;

		run( exe , args ) ;
		return 0 ;
	}
	catch( std::exception & e )
	{
		std::cerr << "exception: " << e.what() << std::endl ;

		// dialog box
		if( !gui.empty() )
		{
			std::stringstream ss ;
			ss << gui << " --message " << sanitised(e.what()) ;
			GDEF_IGNORE_RETURN ::system( ss.str().c_str() ) ;
		}
	}
	return 1 ;
}

