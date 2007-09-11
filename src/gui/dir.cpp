//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include <stdexcept>

Dir::Dir( const std::string & argv0 , bool installed ) :
	m_argv0(argv0)
{
	G::Path exe_dir = G::Path(m_argv0).dirname() ; 
	m_thisdir = ( exe_dir.isRelative() && !exe_dir.hasDriveLetter() ) ? ( cwd() + exe_dir.str() ) : exe_dir ;
	m_thisexe = G::Path( m_thisdir , G::Path(m_argv0).basename() ) ;
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

void Dir::read( std::istream & file )
{
	std::string line ;

	// these are presented by the gui...
	line = G::Str::readLineFrom(file,"\n") ; if( file.good() && !line.empty() ) m_spool = line ;
	line = G::Str::readLineFrom(file,"\n") ; if( file.good() && !line.empty() ) m_config = line ;

	// these allow "make install" to take full control if it needs to...
	line = G::Str::readLineFrom(file,"\n") ; if( file.good() && !line.empty() ) m_pid = line ;
	line = G::Str::readLineFrom(file,"\n") ; if( file.good() && !line.empty() ) m_boot = line ;
	line = G::Str::readLineFrom(file,"\n") ; // was m_startup -- ignored
	line = G::Str::readLineFrom(file,"\n") ; if( file.good() && !line.empty() ) m_desktop = line ;
	line = G::Str::readLineFrom(file,"\n") ; if( file.good() && !line.empty() ) m_login = line ;
	line = G::Str::readLineFrom(file,"\n") ; if( file.good() && !line.empty() ) m_menu = line ;
	line = G::Str::readLineFrom(file,"\n") ; if( file.good() && !line.empty() ) ; // reskit not used

	// this is for completeness only...
	line = G::Str::readLineFrom(file,"\n") ; if( file.good() && !line.empty() ) m_install = line ;
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

/// \file dir.cpp
