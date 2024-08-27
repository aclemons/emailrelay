//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file glogoutput_unix.cpp
///

#include "gdef.h"
#include "glogoutput.h"
#include "glimits.h"
#include <syslog.h>
#include <iostream>

namespace G
{
	namespace LogOutputImp
	{
		int decode( LogOutput::SyslogFacility facility )
		{
			if( facility == LogOutput::SyslogFacility::User ) return LOG_USER ; // NOLINT
			if( facility == LogOutput::SyslogFacility::Daemon ) return LOG_DAEMON ; // NOLINT
			if( facility == LogOutput::SyslogFacility::Mail ) return LOG_MAIL ; // NOLINT
			if( facility == LogOutput::SyslogFacility::Cron ) return LOG_CRON ; // NOLINT
			if( facility == LogOutput::SyslogFacility::Local0 ) return LOG_LOCAL0 ; // NOLINT
			if( facility == LogOutput::SyslogFacility::Local1 ) return LOG_LOCAL1 ; // NOLINT
			if( facility == LogOutput::SyslogFacility::Local2 ) return LOG_LOCAL2 ; // NOLINT
			if( facility == LogOutput::SyslogFacility::Local3 ) return LOG_LOCAL3 ; // NOLINT
			if( facility == LogOutput::SyslogFacility::Local4 ) return LOG_LOCAL4 ; // NOLINT
			if( facility == LogOutput::SyslogFacility::Local5 ) return LOG_LOCAL5 ; // NOLINT
			if( facility == LogOutput::SyslogFacility::Local6 ) return LOG_LOCAL6 ; // NOLINT
			if( facility == LogOutput::SyslogFacility::Local7 ) return LOG_LOCAL7 ; // NOLINT
			return LOG_USER ; // NOLINT
		}
		int decode( LogOutput::Severity severity )
		{
			if( severity == LogOutput::Severity::Warning ) return LOG_WARNING ;
			if( severity == LogOutput::Severity::Error ) return LOG_ERR ;
			if( severity == LogOutput::Severity::InfoSummary ) return LOG_INFO ;
			if( severity == LogOutput::Severity::InfoVerbose ) return LOG_INFO ;
			return LOG_CRIT ;
		}
		int mode( LogOutput::SyslogFacility facility , LogOutput::Severity severity )
		{
			return decode(facility) | decode(severity) ; // NOLINT
		}
	}
}

void G::LogOutput::osoutput( int fd , Severity severity , char * message , std::size_t n )
{
	if( m_config.m_use_syslog && severity != Severity::Debug )
	{
		message[n] = '\0' ; // sic
		::syslog( LogOutputImp::mode(m_config.m_facility,severity) , "%s" , message ) ; // NOLINT
	}

	if( m_config.m_quiet_stderr && (
		severity == Severity::Debug ||
		severity == Severity::InfoVerbose ||
		severity == Severity::InfoSummary ) )
	{
		;
	}
	else
	{
		message[n] = '\n' ; // sic
		GDEF_IGNORE_RETURN ::write( fd , message , n+1U ) ;
	}
}

void G::LogOutput::osinit()
{
	m_handle = 1 ; // pacify -Wunused-private-field
	if( m_config.m_use_syslog )
		::openlog( nullptr , LOG_PID , LogOutputImp::decode(m_config.m_facility) ) ;
}

#ifndef G_LIB_SMALL
void G::LogOutput::register_( const Path & )
{
}
#endif

void G::LogOutput::oscleanup() const noexcept
{
	if( m_config.m_use_syslog )
		::closelog() ;
}

