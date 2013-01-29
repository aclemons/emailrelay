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
// glog.cpp
//

#include "gdef.h"
#include "glog.h"
#include "glogoutput.h"

G::Log::Log( Severity severity , const char * file , int line ) :
	m_severity(severity) ,
	m_file(file) ,
	m_line(line)
{
}

G::Log::~Log()
{
	try
	{
		flush() ;
	}
	catch(...)
	{
	}
}


bool G::Log::active()
{
	LogOutput * output = G::LogOutput::instance() ;
	if( output == NULL )
	{
		return false ;
	}
	else
	{
		// (enable it just to get the original state, then restore it)
		bool a = output->enable(true) ;
		output->enable(a) ;
		return a ;
	}
}

void G::Log::flush()
{
	if( active() )
	{
		G::LogOutput::output( m_severity , m_file , m_line , m_ss.str() ) ;
	}
}

std::ostream & G::Log::operator<<( const char * s )
{
	s = s ? s : "" ;
	return m_ss << s ;
}

std::ostream & G::Log::operator<<( const std::string & s )
{
	return m_ss << s ;
}

/// \file glog.cpp
