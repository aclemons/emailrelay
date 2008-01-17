//
// Copyright (C) 2001-2008 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// dir_mac.cpp
//

#include "gdef.h"
#include "dir.h"
#include "gpath.h"
#include <cstdlib> //getenv

#ifndef G_SPOOLDIR
	#define G_SPOOLDIR ""
#endif

#ifndef G_SYSCONFDIR
	#define G_SYSCONFDIR ""
#endif

#ifndef G_DESTDIR
	#define G_DESTDIR ""
#endif

namespace
{
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

	G::Path desktop_()
	{
		return envPath("HOME") + "Desktop" ;
	}
}

std::string Dir::dotexe()
{
	return std::string() ;
}

G::Path Dir::os_install() const
{
	std::string s( G_DESTDIR ) ;
	return s.empty() ? desktop_() : G::Path(s) ;
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

G::Path Dir::os_pid() const
{
	return "/var/run" ;
}

G::Path Dir::special( const std::string & type )
{
	if( type == "desktop" ) return desktop_() ;
	if( type == "menu" ) return G::Path() ;
	if( type == "login" ) return G::Path() ;
	if( type == "programs" ) return G::Path() ;
	return G::Path() ;
}

G::Path Dir::os_boot() const
{
	return "/Library/StartupItems" ;
}

/// \file dir_mac.cpp
