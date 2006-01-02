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
// glogoutput_unix.cpp
//

#include "gdef.h"
#include "glogoutput.h"
#include <syslog.h>
#include <iostream>

namespace
{
	int decode( G::LogOutput::SyslogFacility facility )
	{
		if( facility == G::LogOutput::User ) return LOG_USER ;
		if( facility == G::LogOutput::Daemon ) return LOG_DAEMON ;
		if( facility == G::LogOutput::Mail ) return LOG_MAIL ;
		if( facility == G::LogOutput::Cron ) return LOG_CRON ;
		// etc...
		return LOG_USER ;
	}
	int decode( G::Log::Severity severity )
	{
		if( severity == G::Log::s_Warning ) return LOG_WARNING ;
		if( severity == G::Log::s_Error ) return LOG_ERR ;
		if( severity == G::Log::s_LogSummary ) return LOG_INFO ;
		if( severity == G::Log::s_LogVerbose ) return LOG_INFO ;
		return LOG_CRIT ;
	}
	int mode( G::LogOutput::SyslogFacility facility , G::Log::Severity severity )
	{
		return decode(facility) | decode(severity) ;
	}
}

void G::LogOutput::rawOutput( G::Log::Severity severity , const char *message )
{
	if( severity != G::Log::s_Debug && m_syslog )
	{
		::syslog( mode(m_facility,severity) , "%s" , message ) ;
	}
	std::cerr << message << std::endl ;
}

void G::LogOutput::init()
{
	if( m_syslog )
		::openlog( NULL , LOG_PID , decode(m_facility) ) ;
}

void G::LogOutput::cleanup()
{
	if( m_syslog )
		::closelog() ;
}

