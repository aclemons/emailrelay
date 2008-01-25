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

Dir::Dir( const std::string & argv0 , bool installed )
{
	G::Path exe_dir = G::Path(argv0).dirname() ; 
	m_thisdir = ( exe_dir.isRelative() && !exe_dir.hasDriveLetter() ) ? ( cwd() + exe_dir.str() ) : exe_dir ;
	m_thisexe = G::Path( m_thisdir , G::Path(argv0).basename() ) ;
	m_tmp = m_thisdir ; // TODO -- check writable
	m_install = installed ? m_thisdir : os_install() ;
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
	// these are presented by the gui...
	m_spool = state.value( "installed-spool-dir" , m_spool ) ;
	m_config = state.value( "installed-config-dir" , m_config ) ;

	// these allow "make install" to take full control if it needs to -- probably not present
	m_pid = state.value( "installed-pid-dir" , m_pid ) ;
	m_boot = state.value( "installed-boot-dir" , m_boot ) ;
	m_desktop = state.value( "installed-desktop-dir" , m_desktop ) ;
	m_login = state.value( "installed-login-dir" , m_login ) ;
	m_menu = state.value( "installed-menu-dir" , m_menu ) ;

	// this is for completeness only -- should never be present in the state file
	m_install = state.value( "installed-dir" , m_install ) ;
}

void Dir::write( std::ostream & stream ) const
{
	State::write( stream , "installed-spool-dir" , m_spool.str() , "" , "\n" ) ;
	State::write( stream , "installed-config-dir" , m_config.str() , "" , "\n" ) ;
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

G::Path Dir::tmp() const
{
	return m_tmp ;
}

G::Path Dir::pid() const
{
	return m_pid ;
}

G::Path Dir::config() const
{
	return m_config ;
}

G::Path Dir::install() const
{
	return m_install ;
}

G::Path Dir::spool() const
{
	return m_spool ;
}

G::Path Dir::boot() const
{
	return m_boot ;
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

G::Path Dir::home() const
{
	return envPath( "HOME" , "~" ) ;
}

/// \file dir.cpp
