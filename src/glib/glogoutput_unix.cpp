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
#include "glimits.h"
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
			if( facility == G::LogOutput::SyslogFacility::Local0 ) return LOG_LOCAL0 ;
			if( facility == G::LogOutput::SyslogFacility::Local1 ) return LOG_LOCAL1 ;
			if( facility == G::LogOutput::SyslogFacility::Local2 ) return LOG_LOCAL2 ;
			if( facility == G::LogOutput::SyslogFacility::Local3 ) return LOG_LOCAL3 ;
			if( facility == G::LogOutput::SyslogFacility::Local4 ) return LOG_LOCAL4 ;
			if( facility == G::LogOutput::SyslogFacility::Local5 ) return LOG_LOCAL5 ;
			if( facility == G::LogOutput::SyslogFacility::Local6 ) return LOG_LOCAL6 ;
			if( facility == G::LogOutput::SyslogFacility::Local7 ) return LOG_LOCAL7 ;
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

void G::LogOutput::osoutput( int fd , G::Log::Severity severity , char * message , std::size_t n )
{
	if( m_config.m_use_syslog && severity != Log::Severity::s_Debug )
	{
		message[n] = '\0' ;
		::syslog( LogOutputImp::mode(m_config.m_facility,severity) , "%s" , message ) ;
	}

	if( m_config.m_quiet_stderr && (
		severity == Log::Severity::s_Debug ||
		severity == Log::Severity::s_InfoVerbose ||
		severity == Log::Severity::s_InfoSummary ) )
	{
		;
	}
	else
	{
		message[n] = '\n' ;
		::write( fd , message , n+1U ) ;
	}
}

void G::LogOutput::osinit()
{
	m_handle = 1 ; // pacify -Wunused-private-field
	if( m_config.m_use_syslog )
		::openlog( nullptr , LOG_PID , LogOutputImp::decode(m_config.m_facility) ) ;
}

void G::LogOutput::register_( const std::string & )
{
}

void G::LogOutput::oscleanup() const noexcept
{
	if( m_config.m_use_syslog )
		::closelog() ;
}

/// \file glogoutput_unix.cpp
