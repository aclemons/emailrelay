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
/// \file guidir_mac.cpp
///

#include "gdef.h"
#include "guidir.h"
#include "gpath.h"
#include "gfile.h"
#include "gdirectory.h"
#include "genvironment.h"
#include "gstrmacros.h"

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
		bool ok( const std::string & s ) ;
		std::string rebase( const std::string & dir ) ;
		G::Path envPath( const std::string & key , const G::Path & default_ ) ;
	}
}

G::Path Gui::Dir::install()
{
	// user expects to say "/Applications" or "~/Applications"
	return DirImp::rebase( "/Applications" ) ;
}

G::Path Gui::Dir::config()
{
	std::string sysconfdir( G_STR(G_SYSCONFDIR) ) ;
	if( sysconfdir.empty() )
		sysconfdir = DirImp::rebase( "/Applications/E-MailRelay" ) ;
	return sysconfdir ;
}

G::Path Gui::Dir::spool()
{
	std::string spooldir( G_STR(G_SPOOLDIR) ) ;
	if( spooldir.empty() )
		spooldir = DirImp::rebase( "/Applications/E-MailRelay/Spool" ) ;
	return spooldir ;
}

G::Path Gui::Dir::pid( const G::Path & )
{
	return DirImp::ok("/var/run") ? "/var/run" : "/tmp" ;
}

G::Path Gui::Dir::desktop()
{
	return home() + "Desktop" ;
}

G::Path Gui::Dir::menu()
{
	return G::Path() ;
}

G::Path Gui::Dir::autostart()
{
	return G::Path() ;
}

G::Path Gui::Dir::home()
{
	return DirImp::envPath( "HOME" , "~" ) ;
}

// ==

bool Gui::DirImp::ok( const std::string & s )
{
	return
		!s.empty() &&
		G::File::exists(G::Path(s)) &&
		G::Directory(G::Path(s)).valid() &&
		G::Directory(G::Path(s)).writeable() ;
}

std::string Gui::DirImp::rebase( const std::string & dir )
{
	static bool use_root = DirImp::ok( "/Applications" ) ;
	return (use_root?"":"~") + dir ;
}

G::Path Gui::DirImp::envPath( const std::string & key , const G::Path & default_ )
{
	return G::Path( G::Environment::get( key , default_.str() ) ) ;
}

