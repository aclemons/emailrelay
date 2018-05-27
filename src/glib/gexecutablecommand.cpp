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
// gexecutablecommand.cpp
//

#include "gdef.h"
#include "gexecutablecommand.h"
#include "gstr.h"
#include <algorithm>

G::ExecutableCommand::ExecutableCommand( const std::string & s )
{
	if( s.find(' ') == std::string::npos ) // optimisation
	{
		m_exe = G::Path(s) ;
	}
	else
	{
		m_args = osSplit( s ) ;

		// take the first part as the path to the exe
		if( m_args.size() )
		{
			m_exe = G::Path( m_args.at(0U) ) ;
			std::rotate( m_args.begin() , m_args.begin()+1U , m_args.end() ) ;
			m_args.pop_back() ; // remove exe
		}
	}

	// do o/s-specific fixups
	if( m_exe != G::Path() && !osNativelyRunnable() )
	{
		osAddWrapper() ;
	}
}

G::ExecutableCommand::ExecutableCommand( const G::Path & exe_ , const G::StringArray & args_ , bool add_wrapper ) :
	m_exe(exe_) ,
	m_args(args_)
{
	if( add_wrapper && m_exe != G::Path() && !osNativelyRunnable() )
	{
		osAddWrapper() ;
	}
}

G::Path G::ExecutableCommand::exe() const
{
	return m_exe ;
}

G::StringArray G::ExecutableCommand::args() const
{
	return m_args ;
}

std::string G::ExecutableCommand::displayString() const
{
	return
		m_args.size() ?
			std::string("[") + m_exe.str() + "] [" + G::Str::join("] [",m_args) + "]" :
			std::string("[") + m_exe.str() + "]" ;
}

void G::ExecutableCommand::add( const std::string & arg )
{
	m_args.push_back( arg ) ;
}

/// \file gexecutablecommand.cpp
