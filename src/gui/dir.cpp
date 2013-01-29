//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// dir.cpp
//
// See also "dir_unix.cpp", "dir_mac.cpp" and "dir_win32.cpp".
//

#include "gdef.h"
#include "dir.h"
#include "gstr.h"
#include "gpath.h"
#include "gdebug.h"
#include "genvironment.h"

G::Path Dir::install()
{
	return os_install() ;
}

G::Path Dir::gui( const G::Path & base )
{
	return os_gui( base ) ;
}

G::Path Dir::icon( const G::Path & base )
{
	return os_icon( base ) ;
}

G::Path Dir::server( const G::Path & base )
{
	return os_server( base ) ;
}

G::Path Dir::thisdir( const std::string & argv0 , const G::Path & cwd_ )
{
	G::Path exe_dir = G::Path(argv0).dirname() ;
	return ( exe_dir.isRelative() && !exe_dir.hasDriveLetter() ) ? ( cwd_ + exe_dir.str() ) : exe_dir ;
}

G::Path Dir::thisexe( const std::string & argv0 , const G::Path & cwd_ )
{
	return G::Path( thisdir(argv0,cwd_) , G::Path(argv0).basename() ) ;
}

G::Path Dir::desktop()
{
	return special( "desktop" ) ;
}

G::Path Dir::login()
{
	return special( "login" ) ;
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

G::Path Dir::bootcopy( const G::Path & boot , const G::Path & install )
{
	return os_bootcopy( boot , install ) ;
}

std::string Dir::env( const std::string & key , const std::string & default_ )
{
	return G::Environment::get( key , default_ ) ;
}

G::Path Dir::envPath( const std::string & key , const G::Path & default_ )
{
	return G::Path( G::Environment::get( key , default_.str() ) ) ;
}

/// \file dir.cpp
