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
// dir_unix.cpp
//

#include "dir.h"
#include "gpath.h"
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
	G::Path kde( const std::string & key )
	{
		G::Strings args;
		args.push_back( "kde-config" ) ;
		args.push_back( key ) ;
		std::string exe = "/usr/bin/kde-config" ; // TODO
		G::Process::ChildProcess child = G::Process::spawn( exe , args ) ;
		child.wait() ;
		std::string s = child.read() ;
		return s ;
	}

	G::Path kdeDesktop()
	{
		return kde("desktop") ;
	}
	
	G::Path kdeAutostart()
	{
		return kde("autostart") ;
	}
}

std::string Dir::dotexe()
{
	return std::string() ;
}

G::Path Dir::install()
{
	std::string s( G_DESTDIR ) ;
	if( s.empty() )
		s = "/usr/sbin" ;
	return prefix(s) ;
}

G::Path Dir::config()
{
	std::string s( G_SYSCONFDIR ) ;
	if( s.empty() )
		s = "/etc" ;
	return prefix(s) ;
}

G::Path Dir::spool()
{
        std::string spooldir( G_SPOOLDIR ) ;
        if( spooldir.empty() )
                spooldir = "/var/spool/emailrelay" ;
        return prefix(spooldir) ;
}

G::Path Dir::cwd()
{
	char buffer[10000] = { '\0' } ;
	const char * p = getcwd( buffer , sizeof(buffer)-1U ) ;
	buffer[sizeof(buffer)-1U] = '\0' ;
	std::string s = p ? std::string(buffer) : std::string(".") ;
	return G::Path( s ) ;
}

G::Path Dir::tooldir()
{
	std::string s( G_LIBEXECDIR ) ;
	if( s.empty() )
		s = "/usr/lib" ;
	return prefix(s) ;
}

G::Path Dir::startup()
{
	return config() + "init.d" ;
}

G::Path Dir::pid()
{
	return oneOf( "/var/run" , "/tmp" ) ;
}

G::Path Dir::special( const std::string & type )
{
	// see "http://standards.freedesktop.org"

	const char * home_p = ::getenv("HOME") ? ::getenv("HOME") : "~" ;
	const char * xdg_data_home_p = ::getenv("XDG_DATA_HOME") ? ::getenv("XDG_DATA_HOME") : "" ;
	const char * xdg_config_home_p = ::getenv("XDG_CONFIG_HOME") ? ::getenv("XDG_CONFIG_HOME") : "" ;
	G::Path home( home_p ) ;
	G::Path data_home = *xdg_data_home_p ? G::Path(xdg_data_home_p) : ( home + ".local" + "share" ) ;
	G::Path config_home = *xdg_config_home_p ? G::Path(xdg_config_home_p) : ( home + ".config" ) ;

	if( type == "desktop" ) return kdeDesktop() == G::Path() ? ( home + "Desktop" ) : kdeDesktop() ;
	if( type == "menu" ) return data_home + "applications" ;
	if( type == "login" ) return kdeAutostart() == G::Path() ? ( config_home + "autostart" ) : kdeAutostart() ; 
	if( type == "lib" ) return tooldir() ;
	if( type == "programs" ) return "/usr/bin" ;
	if( type == "reskit" ) return "" ;
	return G::Path() ;
}

G::Path Dir::boot()
{
	return oneOf( "/etc/init.d" , "/Library/StartupItems" ) ;
}

G::Path Dir::oneOf( std::string d1 , std::string d2 , std::string d3 , std::string d4 , std::string d5 )
{
	if( !d1.empty() && G::File::exists(G::Path(d1)) ) return G::Path(d1) ;
	if( !d2.empty() && G::File::exists(G::Path(d2)) ) return G::Path(d2) ;
	if( !d3.empty() && G::File::exists(G::Path(d3)) ) return G::Path(d3) ;
	if( !d4.empty() && G::File::exists(G::Path(d4)) ) return G::Path(d4) ;
	if( !d5.empty() && G::File::exists(G::Path(d5)) ) return G::Path(d5) ;
	return G::Path() ;
}

