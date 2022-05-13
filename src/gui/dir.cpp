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
/// \file dir.cpp
///
// See also "dir_unix.cpp", "dir_mac.cpp" and "dir_win32.cpp".
//

#include "gdef.h"
#include "dir.h"
#include "gstr.h"
#include "gpath.h"
#include "genvironment.h"

G::Path Dir::install()
{
	return os_install() ;
}

G::Path Dir::desktop()
{
	return special( "desktop" ) ;
}

G::Path Dir::autostart()
{
	return special( "autostart" ) ;
}

G::Path Dir::menu()
{
	return special( "menu" ) ;
}

G::Path Dir::pid( const G::Path & config )
{
	return os_pid( config ) ;
}

G::Path Dir::config()
{
	return os_config() ;
}

G::Path Dir::spool()
{
	return os_spool() ;
}

G::Path Dir::boot()
{
	return os_boot() ;
}

G::Path Dir::envPath( const std::string & key , const G::Path & default_ )
{
	return G::Path( G::Environment::get( key , default_.str() ) ) ;
}

