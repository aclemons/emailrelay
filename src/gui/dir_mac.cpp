//
// Copyright (C) 2001-2015 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gfile.h"
#include "gdirectory.h"

#ifndef G_SBINDIR
	#define G_SBINDIR ""
#endif
#ifndef G_LIBEXECDIR
	#define G_LIBEXECDIR ""
#endif
#ifndef G_EXAMPLESDIR
	#define G_EXAMPLESDIR ""
#endif
#ifndef G_SYSCONFDIR
	#define G_SYSCONFDIR ""
#endif
#ifndef G_MANDIR
	#define G_MANDIR ""
#endif
#ifndef G_DOCDIR
	#define G_DOCDIR ""
#endif
#ifndef G_SPOOLDIR
	#define G_SPOOLDIR ""
#endif
#ifndef G_INITDIR
	#define G_INITDIR ""
#endif

G::Path Dir::os_install()
{
	// user expects to say "/Applications" or "~/Applications"
	return "/Applications" ;
}

G::Path Dir::os_config()
{
	std::string sysconfdir( G_SYSCONFDIR ) ;
	if( sysconfdir.empty() )
		sysconfdir = "/Applications/E-MailRelay" ; // "/Library/Preferences/E-MailRelay"?
	return sysconfdir ;
}

G::Path Dir::os_spool()
{
	std::string spooldir( G_SPOOLDIR ) ;
	if( spooldir.empty() )
		spooldir = "/Library/Mail/Spool" ;
	return spooldir ;
}

G::Path Dir::os_pid( const G::Path & )
{
	return ok("/var/run") ? "/var/run" : "/tmp" ;
}

G::Path Dir::special( const std::string & type )
{
	if( type == "desktop" ) return home()+"Desktop" ;
	if( type == "menu" ) return G::Path() ;
	if( type == "login" ) return G::Path() ;
	if( type == "programs" ) return G::Path() ;
	return G::Path() ;
}

G::Path Dir::os_boot()
{
	std::string s( G_INITDIR ) ;
	if( !s.empty() )
		return s ;
	return "/Library/StartupItems" ;
}

bool Dir::ok( const std::string & s )
{
	return
		!s.empty() &&
		G::File::exists(G::Path(s)) &&
		G::Directory(G::Path(s)).valid() &&
		G::Directory(G::Path(s)).writeable() ;
}

G::Path Dir::home()
{
	return envPath( "HOME" , "~" ) ;
}

G::Path Dir::os_absolute( const G::Path & dir )
{
	G::Path result = dir ;
	if( dir != G::Path() )
	{
		char * p = ::realpath( dir.str().c_str() , NULL ) ; // assume POSIX.1-2008 behaviour
		if( p && *p ) result = G::Path(std::string(p)) ;
		if( p ) ::free( p ) ;
	}
	return result ;
}

/// \file dir_mac.cpp
