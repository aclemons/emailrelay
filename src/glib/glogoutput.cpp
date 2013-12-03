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
// glogoutput.cpp
//

#include "gdef.h"
#include "glogoutput.h"
#include "glimits.h"
#include <sstream>
#include <string>
#include <ctime>
#include <cstdlib>
#include <fstream>

// (note that the implementation here has to be reentrant and using only the standard runtime library)

G::LogOutput::LogOutput( const std::string & prefix , bool enabled , bool summary_log , 
	bool verbose_log , bool debug , bool level , bool timestamp , bool strip ,
	bool use_syslog , const std::string & stderr_replacement , SyslogFacility syslog_facility ) :
		m_prefix(prefix) ,
		m_enabled(enabled) ,
		m_summary_log(summary_log) ,
		m_verbose_log(verbose_log) ,
		m_debug(debug) ,
		m_level(level) ,
		m_strip(strip) ,
		m_syslog(use_syslog) ,
		m_std_err(err(stderr_replacement)) ,
		m_facility(syslog_facility) ,
		m_time(0) ,
		m_timestamp(timestamp) ,
		m_handle(0) ,
		m_handle_set(false)
{
	if( pthis() == NULL )
		pthis() = this ;
	init() ;
}

G::LogOutput::LogOutput( bool enabled_and_summary , bool verbose_and_debug ,
	const std::string & stderr_replacement ) :
		m_enabled(enabled_and_summary) ,
		m_summary_log(enabled_and_summary) ,
		m_verbose_log(verbose_and_debug) ,
		m_debug(verbose_and_debug) ,
		m_level(false) ,
		m_strip(false) ,
		m_syslog(false) ,
		m_std_err(err(stderr_replacement)) ,
		m_facility(User) ,
		m_time(0) ,
		m_timestamp(false) ,
		m_handle(0) ,
		m_handle_set(false)
{
	if( pthis() == NULL )
		pthis() = this ;
	init() ;
}

std::ostream & G::LogOutput::err( const std::string & path_in )
{
	if( !path_in.empty() )
	{
		std::string path = path_in ;
		std::string::size_type pos = path.find("%d") ;
		if( pos != std::string::npos )
			path.replace( pos , 2U , dateString() ) ;

		static std::ofstream file( path.c_str() , std::ios_base::out | std::ios_base::app ) ; 
		// ignore errors
		return file ;
	}
	else
	{
		return std::cerr ;
	}
}

G::LogOutput::~LogOutput()
{
	if( pthis() == this )
		pthis() = NULL ;
	cleanup() ;
}

G::LogOutput * & G::LogOutput::pthis()
{
	static G::LogOutput * p = NULL ;
	return p ;
}

G::LogOutput * G::LogOutput::instance()
{
	return pthis() ;
}

bool G::LogOutput::enable( bool enabled )
{
	bool was_enabled = m_enabled ;
	m_enabled = enabled ;
	return was_enabled ;
}

void G::LogOutput::output( Log::Severity severity , const char * file , int line , const std::string & text )
{
	if( instance() != NULL )
		instance()->doOutput( severity , file , line , text ) ;
}

void G::LogOutput::doOutput( Log::Severity severity , const char * /* file */ , int /* line */ , const std::string & text )
{
	// decide what to do
	bool do_output = m_enabled ;
	if( severity == Log::s_Debug ) 
		do_output = m_enabled && m_debug ;
	else if( severity == Log::s_LogSummary )
		do_output = m_enabled && m_summary_log ;
	else if( severity == Log::s_LogVerbose ) 
		do_output = m_enabled && m_verbose_log ;

	if( do_output )
	{
		// allocate a buffer
		const size_type limit = static_cast<size_type>(limits::log) ;
		std::string buffer ;
		buffer.reserve( (text.length()>limit?limit:text.length()) + 40U ) ;

		// add the preamble to tbe buffer
		std::string::size_type text_pos = 0U ;
		if( m_prefix.length() )
		{
			buffer.append( m_prefix ) ;
			buffer.append( ": " ) ;
		}
		if( m_timestamp )
			buffer.append( timestampString() ) ;
		if( m_level )
			buffer.append( levelString(severity) ) ;
		if( m_strip )
		{
			text_pos = text.find(' ') ;
			if( text_pos == std::string::npos || (text_pos+1U) == text.length() )
				text_pos = 0U ;
			else
				text_pos++ ;
		}

		// add the text to the buffer, with a sanity limit
		size_type text_len = text.length() - text_pos ;
		bool limited = text_len > limit ;
		text_len = text_len > limit ? limit : text_len ;
		buffer.append( text , text_pos , text_len ) ;
		if( limited )
			buffer.append( " ..." ) ;

		// last ditch removal of ansi escape sequences
		while( buffer.find('\033') != std::string::npos )
			buffer[buffer.find('\033')] = '.' ;

		// do the actual output in an o/s-specific manner
		rawOutput( m_std_err , severity , buffer ) ;
	}
}

void G::LogOutput::onAssert()
{
	// no-op
}

std::string G::LogOutput::timestampString()
{
	// use a data member buffer to optimise away calls to localtime() and strftime()
	std::time_t now = std::time(NULL) ;
	if( m_time == 0 || m_time != now )
	{
		m_time = now ;
		static struct std::tm zero_broken_down_time ;
		struct std::tm broken_down_time = zero_broken_down_time ;
		getLocalTime( m_time , &broken_down_time ) ;
		m_time_buffer[0] = '\0' ;
		std::strftime( m_time_buffer, sizeof(m_time_buffer)-1U, "%Y" "%m" "%d." "%H" "%M" "%S: ", &broken_down_time ) ;
		m_time_buffer[sizeof(m_time_buffer)-1U] = '\0' ;
	}
	return std::string(m_time_buffer) ;
}

std::string G::LogOutput::dateString()
{
	static struct std::tm zero_broken_down_time ;
	struct std::tm broken_down_time = zero_broken_down_time ;
	getLocalTime( std::time(NULL) , &broken_down_time ) ;
	char buffer[10] = { 0 } ;
	std::strftime( buffer , sizeof(buffer)-1U , "%Y" "%m" "%d" , &broken_down_time ) ;
	buffer[sizeof(buffer)-1U] = '\0' ;
	return std::string( buffer ) ;
}

std::string G::LogOutput::fileAndLine( const char * file , int line )
{
	if( file != NULL )
	{
		std::string basename( file ) ;
		std::string::size_type slash_pos = basename.find_last_of( "/\\" ) ;
		if( slash_pos != std::string::npos && (slash_pos+1U) < basename.length() )
			basename.erase( 0U , slash_pos+1U ) ;
		return basename + "(" + itoa(line) + "): " ;
	}
	else
	{
		return std::string() ;
	}
}

void G::LogOutput::assertion( const char * file , int line , bool test , const std::string & test_string )
{
	if( !test )
	{
		if( instance() )
			instance()->doAssertion( file , line , test_string ) ;
		else
			std::cerr << "assertion error: " << file << "(" << line << "): " << test_string << std::endl ;
		halt() ;
	}
}

void G::LogOutput::doAssertion( const char * file , int line , const std::string & test_string )
{
	// forward to derived classes -- overrides may safely re-enter this 
	// method since all code in this class is re-entrant
	onAssert() ;

	rawOutput( m_std_err , Log::s_Assertion , 
		std::string() + "Assertion error: " + fileAndLine(file,line) + test_string ) ;
}

void G::LogOutput::halt()
{
	std::abort() ;
}

std::string G::LogOutput::levelString( Log::Severity s )
{
	if( s == Log::s_Debug ) return "debug: " ;
	else if( s == Log::s_LogSummary ) return "info: " ;
	else if( s == Log::s_LogVerbose ) return "info: " ;
	else if( s == Log::s_Warning ) return "warning: " ;
	else if( s == Log::s_Error ) return "error: " ;
	else if( s == Log::s_Assertion ) return "fatal: " ;
	return std::string() ;
}

std::string G::LogOutput::itoa( int n_ )
{
	// diy implementation for speed and portability
	if( n_ < 0 ) return std::string(1U,'0') ;
	char buffer[20] = { '0' , '\0' } ;
	unsigned int buffer_size = sizeof(buffer) ;
	unsigned int n = static_cast<unsigned int>(n_) ;
	n %= 1000000U ;
	bool zero = n == 0U ;
	char * p = buffer + buffer_size - 1U ;
	for( *p-- = '\0' ; n > 0U ; --p , n /= 10U )
		*p = static_cast<char>( '0' + (n % 10U) ) ;
	return zero ? buffer : (p+1U) ;
}

/// \file glogoutput.cpp
