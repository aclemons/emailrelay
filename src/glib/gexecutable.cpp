//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gexecutable.cpp
//

#include "gdef.h"
#include "gexecutable.h"
#include "gstr.h"
#include <algorithm>

G::Executable::Executable( const std::string & s_ )
{
	std::string s( s_ ) ;
	if( s.find(' ') == std::string::npos ) // optimisation
	{
		m_exe = G::Path(s) ;
	}
	else
	{
		// mark escaped spaces using nul
		const std::string null( 1U , '\0' ) ;
		G::Str::replaceAll( s , "\\ " , null ) ;

		// split up on (unescaped) spaces
		G::Str::splitIntoTokens( s , m_args , " " ) ;

		// replace the escaped spaces
		for( G::StringArray::iterator p = m_args.begin() ; p != m_args.end() ; ++p )
		{
			G::Str::replaceAll( *p , null , " " ) ;
		}

		// take the first part as the path to the exe
		if( m_args.size() )
		{
			m_exe = G::Path( m_args.at(0U) ) ;
			std::rotate( m_args.begin() , m_args.begin()+1U , m_args.end() ) ;
			m_args.pop_back() ; // ie. pop_front()
		}
	}

	// do o/s-specific fixups
	if( m_exe != G::Path() && !osNativelyRunnable() )
	{
		osAddWrapper() ;
	}
}

G::Executable::Executable( const G::Path & exe_ , const G::StringArray & args_ ) :
	m_exe(exe_) ,
	m_args(args_)
{
}

G::Path G::Executable::exe() const
{
	return m_exe ;
}

G::StringArray G::Executable::args() const
{
	return m_args ;
}

std::string G::Executable::displayString() const
{
	return
		m_args.size() ?
			std::string("[") + m_exe.str() + "] [" + G::Str::join("] [",m_args) + "]" :
			std::string("[") + m_exe.str() + "]" ;
}

void G::Executable::add( const std::string & arg )
{
	m_args.push_back( arg ) ;
}

/// \file gexecutable.cpp
