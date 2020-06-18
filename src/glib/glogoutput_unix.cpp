//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// glogoutput_unix.cpp
//

#include "gdef.h"
#include "glogoutput.h"
#include <syslog.h>
#include <iostream>

namespace G
{
	namespace LogOutputImp
	{
		int decode( G::LogOutput::SyslogFacility facility )
		{
			if( facility == G::LogOutput::SyslogFacility::User ) return LOG_USER ;
			if( facility == G::LogOutput::SyslogFacility::Daemon ) return LOG_DAEMON ;
			if( facility == G::LogOutput::SyslogFacility::Mail ) return LOG_MAIL ;
			if( facility == G::LogOutput::SyslogFacility::Cron ) return LOG_CRON ;
			// etc...
			return LOG_USER ;
		}
		int decode( G::Log::Severity severity )
		{
			if( severity == G::Log::Severity::s_Warning ) return LOG_WARNING ;
			if( severity == G::Log::Severity::s_Error ) return LOG_ERR ;
			if( severity == G::Log::Severity::s_InfoSummary ) return LOG_INFO ;
			if( severity == G::Log::Severity::s_InfoVerbose ) return LOG_INFO ;
			return LOG_CRIT ;
		}
		int mode( G::LogOutput::SyslogFacility facility , G::Log::Severity severity )
		{
			return decode(facility) | decode(severity) ;
		}
	}
}

void G::LogOutput::open( std::ofstream & file , const std::string & path )
{
	file.open( path.c_str() , std::ios_base::out | std::ios_base::app ) ;
}

void G::LogOutput::rawOutput( std::ostream & std_err , G::Log::Severity severity , const std::string & message )
{
	if( severity != Log::Severity::s_Debug && m_config.m_use_syslog )
	{
		::syslog( LogOutputImp::mode(m_facility,severity) , "%s" , message.c_str() ) ;
	}

	if( !m_config.m_quiet_stderr || severity == Log::Severity::s_Error || severity == Log::Severity::s_Warning )
	{
		std_err << message << std::endl ;
	}
}

void G::LogOutput::init()
{
	if( m_config.m_use_syslog )
		::openlog( nullptr , LOG_PID , LogOutputImp::decode(m_facility) ) ;
}

void G::LogOutput::register_( const std::string & )
{
}

void G::LogOutput::cleanup() const noexcept
{
	if( m_config.m_use_syslog )
		::closelog() ;
}

/// \file glogoutput_unix.cpp
