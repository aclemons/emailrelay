//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gexecutablecommand.cpp
///

#include "gdef.h"
#include "gexecutablecommand.h"
#include "garg.h"
#include "gstr.h"
#include <algorithm>
#include <iterator>

#ifndef G_LIB_SMALL
G::ExecutableCommand::ExecutableCommand( const std::string & s )
{
	if( s.find(' ') == std::string::npos ) // optimisation
	{
		m_exe = s ;
	}
	else
	{
		G::Arg arg ;
		arg.parse( s ) ;
		m_args = arg.array() ;
		if( !m_args.empty() )
		{
			m_exe = m_args.at(0U) ;
			std::rotate( m_args.begin() , m_args.begin()+1U , m_args.end() ) ;
			m_args.pop_back() ; // remove exe
		}
	}
}
#endif

G::ExecutableCommand::ExecutableCommand( const G::Path & exe_ , const G::StringArray & args_ ) :
	m_exe(exe_) ,
	m_args(args_)
{
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
		m_args.empty() ?
			std::string("[") + m_exe.str() + "]" :
			std::string("[") + m_exe.str() + "] [" + Str::join("] [",m_args) + "]" ;
}

void G::ExecutableCommand::add( const std::string & arg )
{
	m_args.push_back( arg ) ;
}

#ifndef G_LIB_SMALL
void G::ExecutableCommand::insert( const G::StringArray & array )
{
	if( !array.empty() )
	{
		m_args.insert( m_args.begin() , m_exe.str() ) ;
		m_args.insert( m_args.begin() , std::next(array.begin()) , array.end() ) ;
		m_exe = m_args.at( 0U ) ;
	}
}
#endif

