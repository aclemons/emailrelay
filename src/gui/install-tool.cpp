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
// install-tool.cpp
//
// usage: install-tool [--show] <config>
//

#include "gstr.h"
#include "gpath.h"
#include "gfile.h"
#include <exception>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#ifdef _WIN32
namespace { void sleep( int n ) { ::Sleep(n*1000U) ; } }
#else
#include <unistd.h>
#endif

typedef std::map<std::string,std::string> Map ;

static Map read( const std::string & path ) ;
static void run( int argc , char * argv [] ) ;
static void show( const Map & map ) ;
static void action( const Map & map ) ;
static void auth_line( std::ostream & stream , bool , const std::string & , 
	const Map & , const std::string & , const std::string & ) ;
static std::string auth( const Map & map , bool , const std::string & ) ;
static std::string commandline( const Map & map ) ;
static std::string unmask( const std::string & , const std::string & ) ;
static std::string value( const Map & map , const std::string & key ) ;
static bool exists( const Map & map , const std::string & key ) ;
static bool yes( const std::string & value ) ;
static std::string rot13( const std::string & in ) ;
static std::string piddir() ;

int main( int argc , char * argv [] )
{
	try
	{
		run( argc , argv ) ;
		return 0 ;
	}
	catch( std::exception & e )
	{
		std::cerr << "exception: " << e.what() << std::endl ;
	}
	catch( ... )
	{
		std::cerr << "unknown exception" << std::endl ;
	}
	return 1 ;
}

Map read( const std::string & path )
{
	std::ifstream file( path.c_str() ) ;
	if( !file.good() )
		throw std::runtime_error( std::string() + "cannot open \"" + path + "\"" ) ;

	Map map ;
	std::string line ;
	while( file.good() )
	{
		G::Str::readLineFrom( file , "\n" , line ) ;
		if( line.empty() || line.find('#') == 0U || line.find_first_not_of(" \t\r") == std::string::npos )
			continue ;

		G::StringArray part ;
		G::Str::splitIntoTokens( line , part , " \t" ) ;
		if( part.size() < 2U )
			continue ;

		map.insert( Map::value_type(part[0],part[1]) ) ;
	}
	return map ;
}

void run( int argc , char * argv [] )
{
	std::string arg1 = argc > 1 ? std::string(argv[1]) : std::string() ;
	std::string arg2 = argc > 2 ? std::string(argv[2]) : std::string() ;
	bool do_show = arg1 == "--show" ;
	std::string config = do_show ? arg2 : arg1 ;
	if( config.empty() )
		throw std::runtime_error( "usage error" ) ;

	Map map = read( config ) ;

	if( do_show )
		show( map ) ;
	else
		action( map ) ;
}

void show( const Map & map )
{
	std::cout << "Command-line:" << std::endl ;
	std::cout << G::Str::wrap(commandline(map)," ","   ") << std::endl ;
	std::cout << "Secrets file:" << std::endl ;
	std::cout << auth(map,true," ") << std::endl ;
}

void action( const Map & )
{
	std::cout << "installing..." << std::endl ;
	sleep( 2 ) ;
	std::cout << "done" << std::endl ;
}

bool exists( const Map & map , const std::string & key )
{
	return map.find(key+":") != map.end() ;
}

std::string value( const Map & map , const std::string & key )
{
	Map::const_iterator p = map.find(key+":") ;
	if( p == map.end() )
		throw std::runtime_error( std::string() + "no such value: " + key ) ;
	return (*p).second ;
}

bool yes( const std::string & value )
{
	return value.find_first_of("yY") == 0U ;
}

std::string piddir()
{
	G::Path var_run( "/var/run" ) ;
	G::Path tmp( "/tmp" ) ;
	return G::File::exists(var_run) ? var_run.str() : tmp.str() ;
}

std::string rot13( const std::string & in )
{
	std::string s( in ) ;
	for( std::string::iterator p = s.begin() ; p != s.end() ; ++p )
	{
		if( *p >= 'a' && *p <= 'z' ) 
			*p = 'a' + ( ( ( *p - 'a' ) + 13U ) % 26U ) ;
		if( *p >= 'A' && *p <= 'Z' )
			*p = 'A' + ( ( ( *p - 'A' ) + 13U ) % 26U ) ;
	}
	return s ;
}

std::string unmask( const std::string & m , const std::string & s )
{
	if( m == "CRAM-MD5" )
	{
		return s ;
	}
	else
	{
		return rot13(s) ;
	}
}

void auth_line( std::ostream & stream , bool show , const std::string & prefix , 
	const Map & map , const std::string & k1 , const std::string & k2 )
{
	if( exists(map,k2+"-name") && !value(map,k2+"-name").empty() )
	{
		stream
			<< prefix
			<< value(map,k1) << " server " 
			<< value(map,k2+"-name") << " "
			<< (show?std::string("*"):unmask(value(map,k1),value(map,k2+"-password")))
			<< std::endl ;
	}
}

std::string auth( const Map & map , bool show , const std::string & prefix )
{
	std::ostringstream ss ;
	if( yes(value(map,"do-pop")) )
	{
		std::string mechanism = value(map,"pop-auth-mechanism") ;
		auth_line( ss , show , prefix , map , "pop-auth-mechanism" , "pop-account-1" ) ;
		auth_line( ss , show , prefix , map , "pop-auth-mechanism" , "pop-account-2" ) ;
		auth_line( ss , show , prefix , map , "pop-auth-mechanism" , "pop-account-3" ) ;
	}
	if( yes(value(map,"do-smtp")) && yes(value(map,"smtp-server-auth")) )
	{
		std::string mechanism = value(map,"smtp-server-auth-mechanism") ;
		auth_line( ss , show , prefix , map , "smtp-server-auth-mechanism" , "smtp-server-account" ) ;
	}
	if( yes(value(map,"do-smtp")) && yes(value(map,"smtp-client-auth")) )
	{
		std::string mechanism = value(map,"smtp-client-auth-mechanism") ;
		auth_line( ss , show , prefix , map , "smtp-client-auth-mechanism" , "smtp-client-account" ) ;
	}
	return ss.str() ;
}

std::string commandline( const Map & map )
{
	std::ostringstream ss ;
	ss << value(map,"install-dir") << "/emailrelay " ;
	ss << "--spool-dir " << value(map,"spool-dir") << " " ;
	ss << "--log --close-stderr " ;
	ss << "--remote-clients " ;
	ss << "--pid-file " << piddir() << "/emailrelay.pid " ;
	if( yes(value(map,"do-smtp")) )
	{
		ss << "--postmaster " ;
		if( yes(value(map,"forward-immediate")) )
		{
			ss << "--immediate " ;
		}
		if( yes(value(map,"forward-poll")) )
		{
			ss << "--poll " ;
			if( value(map,"forward-poll-period") == "minute" )
				ss << "60 " ;
			else if( value(map,"forward-poll-period") == "second" )
				ss << "1 " ;
			else 
				ss << "3600 " ;
		}
		if( value(map,"smtp-server-port") != "25" )
		{
			ss << "--port " << value(map,"smtp-server-port") << " " ;
		}
		if( yes(value(map,"smtp-server-auth")) )
		{
			ss << "--server-auth " << value(map,"config-dir") << "/emailrelay.auth " ;
		}
		ss << "--forward-to " << value(map,"smtp-client-host") << ":" << value(map,"smtp-client-port") << " " ;
		if( yes(value(map,"smtp-client-auth")) )
		{
			ss << "--client-auth " << value(map,"config-dir") << "/emailrelay.auth " ;
		}
	}
	else
	{
		ss << "--no-smtp " ;
	}
	if( yes(value(map,"do-pop")) )
	{
		ss << "--pop " ;
		if( value(map,"pop-port") != "110" )
		{
			ss << "--pop-port " << value(map,"pop-port") << " " ;
		}
		if( yes(value(map,"pop-shared-no-delete")) )
		{
			ss << "--pop-no-delete " ;
		}
		if( yes(value(map,"pop-by-name")) )
		{
			ss << "--pop-by-name " ;
		}
		if( yes(value(map,"pop-by-name-auto-copy")) )
		{
			ss << "--filter " << value(map,"install-dir") << "/emailrelay-filter-copy " ;
		}
	}
	if( yes(value(map,"start-verbose")) )
	{
		ss << "--verbose " ;
	}
	return ss.str() ;
}

