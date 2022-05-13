//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file dir_mac.cpp
///

#include "gdef.h"
#include "dir.h"
#include "gpath.h"
#include "gfile.h"
#include "gdirectory.h"
#include "gstrmacros.h"

#ifndef G_SBINDIR
	#define G_SBINDIR
#endif
#ifndef G_LIBEXECDIR
	#define G_LIBEXECDIR
#endif
#ifndef G_EXAMPLESDIR
	#define G_EXAMPLESDIR
#endif
#ifndef G_SYSCONFDIR
	#define G_SYSCONFDIR
#endif
#ifndef G_MANDIR
	#define G_MANDIR
#endif
#ifndef G_DOCDIR
	#define G_DOCDIR
#endif
#ifndef G_SPOOLDIR
	#define G_SPOOLDIR
#endif
#ifndef G_INITDIR
	#define G_INITDIR
#endif

std::string Dir::rebase( const std::string & dir )
{
	static bool use_root = ok( "/Applications" ) ;
	return (use_root?"":"~") + dir ;
}

G::Path Dir::os_install()
{
	// user expects to say "/Applications" or "~/Applications"
	return rebase( "/Applications" ) ;
}

G::Path Dir::os_config()
{
	std::string sysconfdir( G_STR(G_SYSCONFDIR) ) ;
	if( sysconfdir.empty() )
		sysconfdir = rebase( "/Applications/E-MailRelay" ) ;
	return sysconfdir ;
}

G::Path Dir::os_spool()
{
	std::string spooldir( G_STR(G_SPOOLDIR) ) ;
	if( spooldir.empty() )
		spooldir = rebase( "/Applications/E-MailRelay/Spool" ) ;
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
	if( type == "autostart" ) return G::Path() ;
	if( type == "programs" ) return G::Path() ;
	return G::Path() ;
}

G::Path Dir::os_boot()
{
	std::string s( G_STR(G_INITDIR) ) ;
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

