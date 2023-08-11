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
/// \file glog.cpp
///

#include "gdef.h"
#include "glog.h"
#include "glogoutput.h"
#include "gformat.h"

G::Log::Log( Severity severity , const char * file , int line ) noexcept :
	m_severity(severity) ,
	m_file(file) ,
	m_line(line) ,
	m_logstream(LogOutput::start(m_severity,m_file,m_line))
{
	static_assert( noexcept(LogStream(nullptr)) , "" ) ;
}

G::Log::~Log()
{
	static_assert( noexcept(LogOutput::output(m_logstream)) , "" ) ;
	LogOutput::output( m_logstream ) ;
}

bool G::Log::at( Severity s ) noexcept
{
	static_assert( noexcept(LogOutput::instance()) , "" ) ;
	const LogOutput * log_output = LogOutput::instance() ;

	static_assert( noexcept(log_output->at(s)) , "" ) ;
	return log_output && log_output->at( s ) ;
}

G::LogStream & G::Log::operator<<( const char * s ) noexcept
{
	static_assert( noexcept(m_logstream <<s) , "" ) ;
	s = s ? s : "" ;
	return m_logstream << s ;
}

#ifndef G_LIB_SMALL
G::LogStream & G::Log::operator<<( const std::string & s ) noexcept
{
	static_assert( noexcept(m_logstream <<s) , "" ) ;
	return m_logstream << s ;
}
#endif

#ifndef G_LIB_SMALL
G::LogStream & G::Log::operator<<( const format & f )
{
	return m_logstream << f.str() ;
}
#endif

