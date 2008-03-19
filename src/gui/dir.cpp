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
// dir.cpp
//
// See also "dir_unix.cpp" and "dir_win32.cpp".
//

#include "gdef.h"
#include "dir.h"
#include "gstr.h"
#include "gpath.h"
#include "gdebug.h"
#include "state.h"
#include <cstdlib> //getenv

Dir::Dir( const std::string & argv0 )
{
	G::Path exe_dir = G::Path(argv0).dirname() ; 
	m_thisdir = ( exe_dir.isRelative() && !exe_dir.hasDriveLetter() ) ? ( cwd() + exe_dir.str() ) : exe_dir ;
	m_thisexe = G::Path( m_thisdir , G::Path(argv0).basename() ) ;
	m_spool = os_spool() ;
	m_config = os_config() ;
	m_pid = os_pid() ;
	m_boot = os_boot() ;
	m_desktop = special("desktop") ;
	m_login = special("login") ;
	m_menu = special("menu") ;
}

Dir::~Dir()
{
}

void Dir::read( const State & state )
{
	// these are presented by the gui -- they are normally present in the file
	// because they are written by both "make install" and by the DirectoryPage class
	m_spool = state.value( "dir-spool" , m_spool ) ;
	m_config = state.value( "dir-config" , m_config ) ;

	// these allow "make install" to take full control if it needs to -- probably 
	// not present in the file
	m_pid = state.value( "dir-pid" , os_pid_default(m_pid,m_config) ) ;
	m_boot = state.value( "dir-boot" , m_boot ) ;
	m_desktop = state.value( "dir-desktop" , m_desktop ) ;
	m_login = state.value( "dir-login" , m_login ) ;
	m_menu = state.value( "dir-menu" , m_menu ) ;
}

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

G::Path Dir::thisdir() const
{
	return m_thisdir ;
}

G::Path Dir::thisexe() const
{
	return m_thisexe ;
}

G::Path Dir::desktop() const
{
	return m_desktop ;
}

G::Path Dir::login() const
{
	return m_login ;
}

G::Path Dir::menu() const
{
	return m_menu ;
}

G::Path Dir::pid() const
{
	return m_pid ;
}

G::Path Dir::config( int )
{
	return os_config() ;
}

G::Path Dir::config() const
{
	return m_config ;
}

G::Path Dir::spool() const
{
	return m_spool ;
}

G::Path Dir::boot( int )
{
	return os_boot() ;
}

G::Path Dir::boot() const
{
	return m_boot ;
}

G::Path Dir::bootcopy( const G::Path & boot , const G::Path & install )
{
	return os_bootcopy( boot , install ) ;
}

std::string Dir::env( const std::string & key , const std::string & default_ )
{
	const char * p = ::getenv( key.c_str() ) ;
	return p == NULL ? default_ : std::string(p) ;
}

G::Path Dir::envPath( const std::string & key , const G::Path & default_ )
{
	const char * p = ::getenv( key.c_str() ) ;
	return p == NULL ? default_ : G::Path(std::string(p)) ;
}

G::Path Dir::home()
{
	return envPath( "HOME" , "~" ) ;
}

/// \file dir.cpp
