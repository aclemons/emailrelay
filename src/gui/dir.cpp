//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// dir.cpp
//
// See also "dir_unix.cpp" and "dir_win32.cpp".
//

#include "dir.h"
#include "gpath.h"
#include <stdexcept>

Dir * Dir::m_this = NULL ;

Dir::Dir( const std::string & argv0 , const std::string & prefix ) :
	m_argv0(argv0) ,
	m_prefix(prefix)
{
	if( m_this == NULL )
		m_this = this ;
}

Dir::~Dir()
{
	if( this == m_this )
		m_this = NULL ;
}

Dir * Dir::instance()
{
	if( m_this == NULL )
		throw std::runtime_error("internal error: no instance") ;
	return m_this ;
}

G::Path Dir::thisdir()
{
	// (make some effort to return an absolute path -- not foolproof on windows)
	G::Path exe_dir = G::Path(instance()->m_argv0).dirname() ; 
	return
		( exe_dir.isRelative() && !exe_dir.hasDriveLetter() ) ?
			( cwd() + exe_dir.str() ) :
			exe_dir ;
}

G::Path Dir::thisexe()
{
	return G::Path( thisdir() , G::Path(instance()->m_argv0).basename() ) ;
}

G::Path Dir::desktop()
{
	return prefix(special("desktop")) ;
}

G::Path Dir::login()
{
	return prefix(special("login")) ;
}

G::Path Dir::menu()
{
	return prefix(special("menu")) ;
}

G::Path Dir::reskit()
{
	return prefix(special("reskit")) ;
}

G::Path Dir::tmp()
{
	return thisdir() ; // TODO -- check writable
}

G::Path Dir::prefix( G::Path tail )
{
	std::string p = instance()->m_prefix ;
	return p.empty() ?  tail : ( G::Path(p) + tail.str() ) ;
}

G::Path Dir::prefix( std::string tail )
{
	std::string p = instance()->m_prefix ;
	return p.empty() ?  tail : ( G::Path(p) + tail ) ;
}

