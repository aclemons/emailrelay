//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// glogoutput.cpp
//

#include "gdef.h"
#include "glogoutput.h"
#include <cstdlib>
#include <cstring>
#include <ctime>

G::LogOutput * G::LogOutput::m_this = NULL ;

G::LogOutput::LogOutput( const std::string & prefix , bool enabled , bool summary_log , 
	bool verbose_log , bool debug , bool level , bool timestamp , bool strip ,
	bool use_syslog , SyslogFacility syslog_facility ) :
		m_prefix(prefix) ,
		m_enabled(enabled) ,
		m_summary_log(summary_log) ,
		m_verbose_log(verbose_log) ,
		m_debug(debug) ,
		m_level(level) ,
		m_strip(strip) ,
		m_syslog(use_syslog) ,
		m_facility(syslog_facility) ,
		m_time(0) ,
		m_timestamp(timestamp) ,
		m_handle(0) ,
		m_handle_set(false)
{
	if( m_this == NULL )
		m_this = this ;
	init() ;
}

G::LogOutput::LogOutput( bool enabled_and_summary , bool verbose_and_debug ) :
	m_enabled(enabled_and_summary) ,
	m_summary_log(enabled_and_summary) ,
	m_verbose_log(verbose_and_debug) ,
	m_debug(verbose_and_debug) ,
	m_level(false) ,
	m_strip(false) ,
	m_syslog(false) ,
	m_facility(User) ,
	m_time(0) ,
	m_timestamp(false) ,
	m_handle(0) ,
	m_handle_set(false)
{
	if( m_this == NULL )
		m_this = this ;
	init() ;
}

G::LogOutput::~LogOutput()
{
	if( m_this == this )
		m_this = NULL ;
	cleanup() ;
}

G::LogOutput * G::LogOutput::instance()
{
	return m_this ;
}

bool G::LogOutput::enable( bool enabled )
{
	bool was_enabled = m_enabled ;
	m_enabled = enabled ;
	return was_enabled ;
}

//static
void G::LogOutput::output( Log::Severity severity , const char * file , unsigned int line , const char * text )
{
	if( m_this != NULL )
		m_this->doOutput( severity , file , line , text ) ;
}

void G::LogOutput::doOutput( Log::Severity severity , const char * file , unsigned int line , const char * text )
{
	bool do_output = m_enabled ;
	if( severity == Log::s_Debug ) 
		do_output = m_enabled && m_debug ;
	else if( severity == Log::s_LogSummary )
		do_output = m_enabled && m_summary_log ;
	else if( severity == Log::s_LogVerbose ) 
		do_output = m_enabled && m_verbose_log ;

	if( do_output )
	{
		text = text ? text : "" ;

		char buffer[500U] ;
		buffer[0U] = '\0' ;

		if( severity == Log::s_Debug )
		{
			addFileAndLine( buffer , sizeof(buffer) , file , line ) ;
		}
		else
		{
			if( m_prefix.length() )
				add( buffer , sizeof(buffer) , m_prefix + ": " ) ;

			if( m_timestamp )
				add( buffer , sizeof(buffer) , timestampString() ) ;

			if( m_level )
				add( buffer , sizeof(buffer) , levelString(severity) ) ;

			if( m_strip )
				text = std::strchr(text,' ') ? (std::strchr(text,' ')+1U) : text ;
		}

		add( buffer , sizeof(buffer) , text ) ;

		rawOutput( severity , buffer ) ;
	}
}

void G::LogOutput::onAssert()
{
	// no-op
}

const char * G::LogOutput::timestampString()
{
	std::time_t now = std::time(NULL) ;
	if( m_time == 0 || m_time != now ) // optimise calls to localtime() & strftime()
	{
		m_time = now ;
		struct std::tm * tm_p = std::localtime( &m_time ) ;
		m_time_buffer[0] = '\0' ;
		std::strftime( m_time_buffer , sizeof(m_time_buffer)-1U , "%Y" "%m" "%d." "%H" "%M" "%S: " , tm_p ) ;
		m_time_buffer[sizeof(m_time_buffer)-1U] = '\0' ;
	}
	return m_time_buffer ;
}
//static
void G::LogOutput::addFileAndLine( char *buffer , size_t size , const char *file , int line )
{
	if( file != NULL )
	{
		const char *forward = std::strrchr( file , '/' ) ;
		const char *back = std::strrchr( file , '\\' ) ;
		const char *last = forward > back ? forward : back ;
		const char *basename = last ? (last+1) : file ;

		add( buffer , size , basename ) ;
		add( buffer , size , "(" ) ;
		char b[15U] ;
		add( buffer , sizeof(buffer) , itoa(b,sizeof(b),line) ) ;
		add( buffer , sizeof(buffer) , "): " ) ;
	}
}

//static
void G::LogOutput::add( char * buffer , size_t size , const std::string & p )
{
	add( buffer , size , p.c_str() ) ;
}

//static
void G::LogOutput::add( char * buffer , size_t size , const char * p )
{
	std::strncat( buffer+std::strlen(buffer) , p , size-std::strlen(buffer)-1U ) ;
}

void G::LogOutput::assertion( const char *file , unsigned line , bool test , const char *test_string )
{
	if( !test )
	{
		if( instance() )
			instance()->doAssertion( file , line , test_string ) ;
		halt() ;
	}
}

void G::LogOutput::doAssertion( const char * file , unsigned line , const char * test_string )
{
	char buffer[100U] ;
	std::strcpy( buffer , "Assertion error: " ) ;
	size_t size = sizeof(buffer) - 10U ; // -10 for luck
	if( file )
	{
		addFileAndLine( buffer , size , file , line ) ;
	}
	if( test_string )
	{
		add( buffer , size , test_string ) ;
	}

	// forward to derived classes -- these
	// overrides may safely re-enter this method --
	// all code in this class is re-entrant
	//
	onAssert() ;

	rawOutput( Log::s_Assertion , buffer ) ;
}

//static
void G::LogOutput::halt()
{
	abort() ;
}

//static
const char * G::LogOutput::levelString( Log::Severity s )
{
	if( s == Log::s_Debug ) return "debug: " ;
	else if( s == Log::s_LogSummary ) return "info: " ;
	else if( s == Log::s_LogVerbose ) return "info: " ;
	else if( s == Log::s_Warning ) return "warning: " ;
	else if( s == Log::s_Error ) return "error: " ;
	else if( s == Log::s_Assertion ) return "fatal: " ;
	return "" ;
}

//static
const char * G::LogOutput::itoa( char * buffer , size_t buffer_size , unsigned int n )
{
	buffer[0U] = '0' ; buffer[1U] = '\0' ;
	n %= 1000000U ;
	bool zero = n == 0U ;
	char * p = buffer + buffer_size - 1U ;
	for( *p-- = '\0' ; n > 0U ; --p , n /= 10U )
		*p = '0' + (n % 10U) ;
	return zero ? buffer : (p+1U) ;
}
