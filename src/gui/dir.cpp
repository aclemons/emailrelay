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
#include <cstdlib> //getenv

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
	G::Path dummy ;
	bool plain = true ;

	// these are presented by the gui...
	read( m_spool , file , plain ) ;
	read( m_config , file , plain ) ;

	// these allow "make install" to take full control if it needs to...
	read( m_pid , file , plain ) ;
	read( m_boot , file , plain ) ;
	read( dummy , file , plain ) ; // was m_startup -- ignored
	read( m_desktop , file , plain ) ;
	read( m_login , file , plain ) ;
	read( m_menu , file , plain ) ;
	read( dummy , file , plain ) ; // was reskit -- ignored

	// this is for completeness only...
	read( m_install , file , plain ) ;
}

void Dir::read( G::Path & value , std::istream & file , bool & plain )
{
	// This supports two file formats: the old format is a simple text file
	// with one value on each line and the new format uses hash for comment
	// lines and values given as "key=value" (although the key is ignored 
	// for now).

	while( file.good() )
	{
		std::string line = G::Str::readLineFrom( file , "\n" ) ;
		if( file.good() && !line.empty() ) 
		{
			G::Str::trim( line , G::Str::ws() ) ;
			if( line.at(0U) == '#' || line.at(0U) == '@' )
			{
				if( plain )
					G_DEBUG( "Dir::read: not a plain state file" ) ;
				plain = false ;
				continue ;
			}
			std::string::size_type pos = line.find( '=' ) ;
			if( plain || pos != std::string::npos )
			{
				value = plain ? line : G::Str::tail(line,pos) ;
				G_DEBUG( "Dir::read: state file value [" << value << "]" ) ;
				break ;
			}
		}
		if( plain )
			break ;
	}
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
