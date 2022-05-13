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
/// \file dir_unix.cpp
///

#include "gdef.h"
#include "dir.h"
#include "gpath.h"
#include "gstr.h"
#include "gdirectory.h"
#include "gnewprocess.h"
#include "gfile.h"
#include "gstrmacros.h"
#include <stdexcept>
#include <unistd.h>

// these directories from the makefile could be used in preference
// to the installer's runtime base directory on the assumption that
// on unix we always install using "make install" and only ever
// run the installer to reconfigure
//
#ifndef G_SBINDIR
	#define G_SBINDIR
#endif
#ifndef G_ICONDIR
	#define G_ICONDIR
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

namespace
{
	std::string run( const std::string & exe , const std::string & arg_1 = std::string() ,
		const std::string & arg_2 = std::string() ,
		const std::string & arg_3 = std::string() ,
		const std::string & arg_4 = std::string() )
	{
		G::StringArray args ;
		if( !arg_1.empty() ) args.push_back( arg_1 ) ;
		if( !arg_2.empty() ) args.push_back( arg_2 ) ;
		if( !arg_3.empty() ) args.push_back( arg_3 ) ;
		if( !arg_4.empty() ) args.push_back( arg_4 ) ;
		G::NewProcess child( exe , args ) ;
		return child.waitable().wait().output() ;
	}

	G::Path kde( const std::string & key , const G::Path & default_ )
	{
		G::Path result = run( "/usr/bin/kde4-config" , "kde4-config" , "--userpath" , key ) ;
		return result.empty() ? default_ : result ;
	}

	G::Path xdg( const std::string & key , const G::Path & default_ )
	{
		G::Path result = run( "/usr/dir/xdg-user-dir" , "xdg-user-dir" , key ) ;
		return result.empty() ? default_ : result ;
	}

	G::Path queryDesktop( const G::Path & default_ = G::Path() )
	{
		return kde( "desktop" , xdg("DESKTOP",default_) ) ;
	}

	G::Path queryAutostart( const G::Path & default_ = G::Path() )
	{
		return kde( "autostart" , default_ ) ;
	}
}

G::Path Dir::os_install()
{
	// this is what to present to the user as the
	// default base of the install

	return "/usr" ;
}

G::Path Dir::os_config()
{
	std::string sysconfdir( G_STR(G_SYSCONFDIR) ) ;
	if( sysconfdir.empty() )
		sysconfdir = "/etc" ;
	return sysconfdir ;
}

G::Path Dir::os_spool()
{
	std::string spooldir( G_STR(G_SPOOLDIR) ) ;
	if( spooldir.empty() )
		spooldir = "/var/spool/emailrelay" ;
	return spooldir ;
}

G::Path Dir::os_pid( const G::Path & )
{
	return oneOf( "/run" , "/var/run" , "/tmp" ) ;
}

G::Path Dir::special( const std::string & type )
{
	// see "http://standards.freedesktop.org"

	G::Path desktop = queryDesktop( home()+"Desktop" ) ; // see also "xdg-desktop-icon install"
	G::Path menu = envPath("XDG_DATA_HOME",home()+".local"+"share") + "applications" ; // see also "xdg-desktop-menu install"
	G::Path autostart = queryAutostart() ; // default was envPath("XDG_CONFIG_HOME",home()+".config")+"autostart"
	G::Path programs = "/usr/bin" ;

	if( type == "desktop" ) return desktop ;
	if( type == "menu" ) return menu ;
	if( type == "autostart" ) return autostart ;
	if( type == "programs" ) return programs ;
	return G::Path() ;
}

G::Path Dir::os_boot()
{
	std::string s( G_STR(G_INITDIR) ) ;
	if( !s.empty() )
		return s ;
	return "/etc/init.d" ;
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
	if( ok(d5) ) return G::Path(d5) ;
	return G::Path() ;
}

G::Path Dir::home()
{
	return envPath( "HOME" , "~" ) ;
}
