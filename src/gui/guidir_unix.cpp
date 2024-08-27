//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file guidir_unix.cpp
///

#include "gdef.h"
#include "guidir.h"
#include "gpath.h"
#include "gstr.h"
#include "gdirectory.h"
#include "gnewprocess.h"
#include "gfile.h"
#include "gstrmacros.h"
#include <stdexcept>
#include <unistd.h>

#ifndef G_SYSCONFDIR
	#define G_SYSCONFDIR
#endif
#ifndef G_SPOOLDIR
	#define G_SPOOLDIR
#endif

namespace Gui
{
	namespace DirImp
	{
		std::string run( const std::string & exe , const std::string & arg_1 = {} ,
			const std::string & arg_2 = {} ,
			const std::string & arg_3 = {} ,
			const std::string & arg_4 = {} ) ;
		G::Path kde( const std::string & key , const G::Path & default_ ) ;
		G::Path xdg( const std::string & key , const G::Path & default_ ) ;
		G::Path desktop( const G::Path & default_ = G::Path() ) ;
		G::Path autostart( const G::Path & default_ = G::Path() ) ;
		G::Path oneOf( std::string , std::string = {} , std::string = {} , std::string = {} , std::string = {} ) ;
		G::Path envPath( const std::string & , const G::Path & = G::Path() ) ;
		G::Path home() ;
		bool ok( const std::string & ) ;
	}
}

G::Path Gui::Dir::install()
{
	// presented to the user as the default base of the install
	return "/usr" ;
}

G::Path Gui::Dir::config()
{
	std::string sysconfdir( G_STR(G_SYSCONFDIR) ) ; // NOLINT readability-redundant-string-init
	if( sysconfdir.empty() )
		sysconfdir = "/etc" ;
	return sysconfdir ;
}

G::Path Gui::Dir::spool()
{
	std::string spooldir( G_STR(G_SPOOLDIR) ) ; // NOLINT readability-redundant-string-init
	if( spooldir.empty() )
		spooldir = "/var/spool/emailrelay" ;
	return spooldir ;
}

G::Path Gui::Dir::pid( const G::Path & )
{
	return DirImp::oneOf( "/run" , "/var/run" , "/tmp" ) ;
}

G::Path Gui::Dir::desktop()
{
	return DirImp::desktop( DirImp::home()/"Desktop" ) ;
}

G::Path Gui::Dir::menu()
{
	// see also "xdg-desktop-menu install"
	return DirImp::envPath("XDG_DATA_HOME",DirImp::home()/".local"/"share") / "applications" ;
}

G::Path Gui::Dir::autostart()
{
	return DirImp::autostart() ;
}

G::Path Gui::Dir::home()
{
	return DirImp::home() ;
}

// ==

G::Path Gui::DirImp::desktop( const G::Path & default_ )
{
	return kde( "desktop" , xdg("DESKTOP",default_) ) ;
}

G::Path Gui::DirImp::autostart( const G::Path & default_ )
{
	return kde( "autostart" , default_ ) ;
}

std::string Gui::DirImp::run( const std::string & exe , const std::string & arg_1 ,
	const std::string & arg_2 , const std::string & arg_3 , const std::string & arg_4 )
{
	G::StringArray args ;
	if( !arg_1.empty() ) args.push_back( arg_1 ) ;
	if( !arg_2.empty() ) args.push_back( arg_2 ) ;
	if( !arg_3.empty() ) args.push_back( arg_3 ) ;
	if( !arg_4.empty() ) args.push_back( arg_4 ) ;

	G::NewProcess child( exe , args , G::NewProcess::Config() .set_env(G::Environment::inherit()) ) ;
	return G::Str::head( child.waitable().wait().output() , "\n" , false ) ;
}

G::Path Gui::DirImp::kde( const std::string & key , const G::Path & default_ )
{
	G::Path result = run( "/usr/bin/kde4-config" , "--userpath" , key ) ;
	return result.empty() ? default_ : result ;
}

G::Path Gui::DirImp::xdg( const std::string & key , const G::Path & default_ )
{
	G::Path result = run( "/usr/bin/xdg-user-dir" , key ) ;
	return result.empty() ? default_ : result ;
}

G::Path Gui::DirImp::home()
{
	return envPath( "HOME" , "~" ) ;
}

G::Path Gui::DirImp::envPath( const std::string & key , const G::Path & default_ )
{
	return G::Environment::getPath( key , default_ ) ;
}

G::Path Gui::DirImp::oneOf( std::string d1 , std::string d2 , std::string d3 , std::string d4 , std::string d5 )
{
	if( ok(d1) ) return G::Path(d1) ;
	if( ok(d2) ) return G::Path(d2) ;
	if( ok(d3) ) return G::Path(d3) ;
	if( ok(d4) ) return G::Path(d4) ;
	if( ok(d5) ) return G::Path(d5) ;
	return G::Path() ;
}

bool Gui::DirImp::ok( const std::string & s )
{
	return
		!s.empty() &&
		G::File::exists(G::Path(s)) &&
		G::Directory(G::Path(s)).valid() &&
		G::Directory(G::Path(s)).writeable() ;
}

