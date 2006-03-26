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
// Used by the gui to do installation/configuration steps.
//
// usage: install-tool [--show] <config>
//

#include "gstr.h"
#include "gpath.h"
#include "gfile.h"
#include "garg.h"
#include "ggetopt.h"
#include "glogoutput.h"
#include <exception>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <string>
#include <sstream>
#include <utility>
#include <map>
#ifdef _WIN32
namespace { void sleep( int n ) { ::Sleep(n*1000U) ; } }
#else
#include <unistd.h>
#endif

typedef std::map<std::string,std::string> Map ;

static Map read( const std::string & path ) ;
static int run( int argc , char * argv [] ) ;
static void show( const Map & map ) ;
static void action( const Map & map ) ;
static void secrets_line( std::ostream & stream , bool , const std::string & , 
	const Map & , const std::string & , const std::string & , const std::string & ) ;
static std::string secrets_file( const Map & map , bool , const std::string & ) ;
static std::string secrets_filename( const Map & map ) ;
static std::string config_file( const Map & map , const std::string & ) ;
static std::string config_filename( const Map & map ) ;
static std::string commandline_string( const Map & map ) ;
std::pair<std::string,Map> commandline_map( const Map & map ) ;
static std::string unmask( const std::string & , const std::string & ) ;
static std::string value( const Map & map , const std::string & key ) ;
static bool exists( const Map & map , const std::string & key ) ;
static bool yes( const std::string & value ) ;
static std::string rot13( const std::string & in ) ;

int main( int argc , char * argv [] )
{
	try
	{
		return run( argc , argv ) ;
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
		if( part.size() == 0U )
			continue ;
		if( part.size() == 1U )
			part.push_back( std::string() ) ;

		map.insert( Map::value_type(part[0],part[1]) ) ;
	}
	return map ;
}

int run( int argc , char * argv [] )
{
	G::Arg args( argc , argv ) ;
	G::GetOpt getopt( args , 
		"h/help/show this help text and exit/0//1|"
		"f/file/specify input file/1/input-file/1|"
		"d/debug/show debug messages if compiled-in/0//1|"
		"s/show/show what needs doing wihtout doing it/0//1" ) ;
	if( getopt.hasErrors() )
	{
		getopt.showErrors( std::cerr ) ;
		return 2 ;
	}
	if( getopt.args().c() != 1U )
	{
		throw std::runtime_error( "usage error" ) ;
	}
	if( getopt.contains("help") )
	{
		getopt.showUsage( std::cout , std::string() , false ) ;
		return 0 ;
	}
	G::LogOutput log_ouptut( getopt.contains("debug") ) ;
	bool do_show = getopt.contains("show") ;
	std::string config = getopt.contains("file") ? getopt.value("file") : std::string("install.cfg") ;

	Map map = read( config ) ;

	if( do_show )
		show( map ) ;
	else
		action( map ) ;

	return 0 ;
}

void show( const Map & map )
{
	std::cout << "Command-line:" << std::endl ;
	std::cout << G::Str::wrap(commandline_string(map)," ","   ") << std::endl ;
	if( ! config_filename(map).empty() )
	{
		std::cout << "Startup file (" << config_filename(map) << "):" << std::endl ;
		std::cout << config_file(map," ") << std::endl ;
	}
	if( ! secrets_filename(map).empty() )
	{
		std::cout << "Secrets file (" << secrets_filename(map) << "):" << std::endl ;
		std::cout << secrets_file(map,true," ") << std::endl ;
	}
}

void action( const Map & )
{
	std::cout << "installing..." << std::endl ;
	sleep( 1 ) ;
	std::cout << "running..." << std::endl ;
	sleep( 1 ) ;
	std::cout << "running..." << std::endl ;
	sleep( 1 ) ;
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

void secrets_line( std::ostream & stream , bool show , const std::string & prefix , 
	const Map & map , const std::string & side , const std::string & k1 , const std::string & k2 )
{
	if( exists(map,k2+"-name") && !value(map,k2+"-name").empty() )
	{
		stream
			<< prefix
			<< value(map,k1) << " " << side << " " 
			<< value(map,k2+"-name") << " "
			<< (show?std::string("..."):unmask(value(map,k1),value(map,k2+"-password")))
			<< std::endl ;
	}
}

std::string config_file( const Map & map_in , const std::string & prefix )
{
	std::ostringstream ss ;
	std::pair<std::string,Map> pair = commandline_map( map_in ) ;
	for( Map::iterator p = pair.second.begin() ; p != pair.second.end() ; ++p )
	{
		ss << prefix << (*p).first ;
		if( ! (*p).second.empty() )
			ss << " " << (*p).second ;
		ss << std::endl ;
	}
	return ss.str() ;
}

std::string config_filename( const Map & map_in )
{
 #ifdef _WIN32
	return std::string() ;
 #else
	return G::Path(value(map_in,"config-dir"),"emailrelay.conf").str() ;
 #endif
}

std::string secrets_file( const Map & map , bool show , const std::string & prefix )
{
	std::ostringstream ss ;
	if( yes(value(map,"do-pop")) )
	{
		std::string mechanism = value(map,"pop-auth-mechanism") ;
		secrets_line( ss , show , prefix , map , "server" , "pop-auth-mechanism" , "pop-account-1" ) ;
		secrets_line( ss , show , prefix , map , "server" , "pop-auth-mechanism" , "pop-account-2" ) ;
		secrets_line( ss , show , prefix , map , "server" , "pop-auth-mechanism" , "pop-account-3" ) ;
	}
	if( yes(value(map,"do-smtp")) && yes(value(map,"smtp-server-auth")) )
	{
		std::string mechanism = value(map,"smtp-server-auth-mechanism") ;
		secrets_line( ss , show , prefix , map , "server" , "smtp-server-auth-mechanism" , "smtp-server-account" ) ;
		if( ! value(map,"smtp-server-trust").empty() )
		{
			ss << prefix << "NONE server " << value(map,"smtp-server-trust") << " trusted" << std::endl ;
		}
	}
	if( yes(value(map,"do-smtp")) && yes(value(map,"smtp-client-auth")) )
	{
		std::string mechanism = value(map,"smtp-client-auth-mechanism") ;
		secrets_line( ss , show , prefix , map , "client" , "smtp-client-auth-mechanism" , "smtp-client-account" ) ;
	}
	return ss.str() ;
}

std::string secrets_filename( const Map & map )
{
	if( yes(value(map,"do-pop")) || 
		( yes(value(map,"do-smtp")) && yes(value(map,"smtp-server-auth")) ) ||
		( yes(value(map,"do-smtp")) && yes(value(map,"smtp-client-auth")) ) )
	{
		return G::Path(value(map,"config-dir"),"emailrelay.auth").str() ;
	}
	else
	{
		return std::string() ;
	}
}

std::string commandline_string( const Map & map_in )
{
	std::ostringstream ss ;
	std::pair<std::string,Map> pair = commandline_map( map_in ) ;
	ss << pair.first << " " ;
	const char * sep = "" ;
	for( Map::iterator p = pair.second.begin() ; p != pair.second.end() ; ++p , sep = " " )
	{
		ss << sep << "--" << (*p).first ;
		if( ! (*p).second.empty() )
			ss << " " << (*p).second ;
	}
	return ss.str() ;
}

std::pair<std::string,Map> commandline_map( const Map & map )
{
	Map out ;
	std::string path = G::Path(value(map,"install-dir"),"emailrelay").str() ;
	out["spool-dir"] = value(map,"spool-dir") ;
	out["log"] ;
	out["close-stderr"] ;
	out["remote-clients"] ;
	out["pid-file"] = G::Path(value(map,"pid-dir"),"emailrelay.pid").str() ;
	if( yes(value(map,"do-smtp")) )
	{
		if( yes(value(map,"forward-immediate")) )
		{
			out["immediate"] ;
		}
		if( yes(value(map,"forward-poll")) )
		{
			if( value(map,"forward-poll-period") == "minute" )
				out["poll"] = "60" ;
			else if( value(map,"forward-poll-period") == "second" )
				out["poll"] = "1" ;
			else 
				out["poll"] = "3600" ;
		}
		if( value(map,"smtp-server-port") != "25" )
		{
			out["port"] = value(map,"smtp-server-port") ;
		}
		if( yes(value(map,"smtp-server-auth")) )
		{
			out["server-auth"] = G::Path(value(map,"config-dir"),"emailrelay.auth").str() ;
		}
		out["forward-to"]  = value(map,"smtp-client-host") + ":" + value(map,"smtp-client-port") ;
		if( yes(value(map,"smtp-client-auth")) )
		{
			out["client-auth"] = G::Path(value(map,"config-dir"),"emailrelay.auth").str() ;
		}
	}
	else
	{
		out["no-smtp"] ;
	}
	if( yes(value(map,"do-pop")) )
	{
		out["pop"] ;
		if( value(map,"pop-port") != "110" )
		{
			out["pop-port"] = value(map,"pop-port") ;
		}
		if( yes(value(map,"pop-shared-no-delete")) )
		{
			out["pop-no-delete"] ;
		}
		if( yes(value(map,"pop-by-name")) )
		{
			out["pop-by-name"] ;
		}
		if( yes(value(map,"pop-by-name-auto-copy")) )
		{
			out["filter"] = G::Path(value(map,"install-dir"),"emailrelay-filter-copy").str() ;
		}
	}
	if( yes(value(map,"logging-verbose")) )
	{
		out["verbose"] ;
	}
	if( yes(value(map,"logging-debug")) )
	{
		out["debug"] ;
	}
	if( yes(value(map,"logging-syslog")) )
	{
		out["syslog"] ;
	}
	if( yes(value(map,"listening-remote")) )
	{
		out["remote-clients"] ;
	}
	if( ! value(map,"listening-interface").empty() )
	{
		out["interface"] = value(map,"listening-interface") ;
	}
	return std::make_pair(path,out) ;
}

