//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// dir_unix.cpp
//

#include "dir.h"
#include "gpath.h"
#include "gdirectory.h"
#include "gprocess.h"
#include "gfile.h"
#include <cstdlib> //getenv
#include <stdexcept>
#include <unistd.h>

#ifndef G_SPOOLDIR
	#define G_SPOOLDIR ""
#endif

#ifndef G_SYSCONFDIR
	#define G_SYSCONFDIR ""
#endif

#ifndef G_LIBEXECDIR
	#define G_LIBEXECDIR ""
#endif

#ifndef G_DESTDIR
	#define G_DESTDIR ""
#endif

namespace
{
	G::Path kde( const std::string & key , const G::Path & default_ )
	{
		std::string exe = "/usr/bin/kde-config" ; // TODO ?
		G::Strings args ;
		args.push_back( "kde-config" ) ;
		args.push_back( "--userpath" ) ;
		args.push_back( key ) ;

		G::Process::ChildProcess child = G::Process::spawn( exe , args ) ;
		child.wait() ;
		std::string s = child.read() ;
		return s.empty() ? default_ : G::Path(s) ;
	}

	G::Path kdeDesktop( const G::Path & default_ = G::Path() )
	{
		return kde( "desktop" , default_ ) ;
	}
	
	G::Path kdeAutostart( const G::Path & default_ = G::Path() )
	{
		return kde( "autostart" , default_ ) ;
	}

	std::string env( const std::string & key , const std::string & default_ = std::string() )
	{
		const char * p = ::getenv( key.c_str() ) ;
		return p == NULL ? default_ : std::string(p) ;
	}

	G::Path envPath( const std::string & key , const G::Path & default_ = std::string() )
	{
		const char * p = ::getenv( key.c_str() ) ;
		return p == NULL ? default_ : G::Path(std::string(p)) ;
	}
}

std::string Dir::dotexe()
{
	return std::string() ;
}

G::Path Dir::os_install() const
{
	std::string s( G_DESTDIR ) ;
	if( s.empty() )
		s = "/usr/sbin" ;
	return s ;
}

G::Path Dir::os_config() const
{
	std::string s( G_SYSCONFDIR ) ;
	if( s.empty() )
		s = "/etc" ;
	return s ;
}

G::Path Dir::os_spool() const
{
        std::string spooldir( G_SPOOLDIR ) ;
        if( spooldir.empty() )
                spooldir = "/var/spool/emailrelay" ;
        return spooldir ;
}

G::Path Dir::cwd()
{
	char buffer[10000] = { '\0' } ;
	const char * p = getcwd( buffer , sizeof(buffer)-1U ) ;
	buffer[sizeof(buffer)-1U] = '\0' ;
	std::string s = p ? std::string(buffer) : std::string(".") ;
	return G::Path( s ) ;
}

G::Path Dir::os_startup() const
{
	return config() + "init.d" ;
}

G::Path Dir::os_pid() const
{
	return oneOf( "/var/run" , "/tmp" ) ;
}

G::Path Dir::special( const std::string & type )
{
	// see "http://standards.freedesktop.org"

	G::Path home = envPath( "HOME" , "~" ) ;
	G::Path data_home = envPath( "XDG_DATA_HOME" , home + ".local" + "share" ) ;
	G::Path config_home = envPath( "XDG_CONFIG_HOME" , home + ".config" ) ;

	G::Path desktop = kdeDesktop( home + "Desktop" ) ;
	G::Path menu = data_home + "applications" ;
	G::Path login = kdeAutostart( config_home + "autostart" ) ;
	G::Path programs = "/usr/bin" ;
	G::Path reskit ;

	if( type == "desktop" ) return desktop ;
	if( type == "menu" ) return menu ;
	if( type == "login" ) return login ;
	if( type == "programs" ) return programs ;
	if( type == "reskit" ) return reskit ;
	return G::Path() ;
}

G::Path Dir::os_boot() const
{
	return oneOf( "/etc/init.d" , "/Library/StartupItems" ) ;
}

bool Dir::ok( const std::string & s )
{
	return 
		!s.empty() && 
		G::File::exists(G::Path(s)) && 
		G::Directory(G::Path(s)).valid() && 
		G::Directory(G::Path(s)).writeable() ;
}

G::Path Dir::oneOf( std::string d1 , std::string d2 , std::string d3 , std::string d4 , std::string d5 )
{
	if( ok(d1) ) return G::Path(d1) ;
	if( ok(d2) ) return G::Path(d2) ;
	if( ok(d3) ) return G::Path(d3) ;
	if( ok(d4) ) return G::Path(d4) ;
	return G::Path() ;
}

/// \file dir_unix.cpp
