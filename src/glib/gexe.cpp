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
// gexe.cpp
//

#include "gdef.h"
#include "gexe.h"
#include "gstr.h"

G::Executable::Executable( const G::Path & exe ) :
	m_exe(exe)
{
}

G::Executable::Executable( const std::string & s )
{
	if( s.find(' ') == std::string::npos ) // optimisation
	{
		m_exe = G::Path(s) ;
	}
	else
	{
		const std::string null( 1U , '\0' ) ;
		std::string line( s ) ;
		G::Str::replaceAll( line , "\\ " , null ) ;
		G::Str::splitIntoTokens( line , m_args , " " ) ;
		for( G::Strings::iterator p = m_args.begin() ; p != m_args.end() ; ++p )
		{
			G::Str::replaceAll( *p , null , " " ) ;
		}
		if( m_args.size() )
		{
			m_exe = G::Path( m_args.front() ) ;
			m_args.pop_front() ;
		}
	}
}

G::Path G::Executable::exe() const
{
	return m_exe ;
}

G::Strings G::Executable::args() const
{
	return m_args ;
}

