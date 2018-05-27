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
// glogoutput.cpp
//

#include "gdef.h"
#include "gdatetime.h"
#include "glogoutput.h"
#include "genvironment.h"
#include "glimits.h"
#include <sstream>
#include <string>
#include <ctime>
#include <cstdlib>
#include <fstream>

// (note that this code cannot use any library code that might do logging)

G::LogOutput::LogOutput( const std::string & prefix , bool enabled , bool summary_log ,
	bool verbose_log , bool debug , bool with_level , bool with_timestamp , bool strip ,
	bool use_syslog , const std::string & stderr_replacement , SyslogFacility syslog_facility ) :
		m_prefix(prefix) ,
		m_enabled(enabled) ,
		m_summary_log(summary_log) ,
		m_verbose_log(verbose_log) ,
		m_quiet(false) ,
		m_debug(debug) ,
		m_level(with_level) ,
		m_strip(strip) ,
		m_syslog(use_syslog) ,
		m_std_err(err(stderr_replacement)) ,
		m_facility(syslog_facility) ,
		m_time(0) ,
		m_timestamp(with_timestamp) ,
		m_handle(0) ,
		m_handle_set(false)
{
	if( pthis() == nullptr )
		pthis() = this ;
	init() ;
	groups( G::Environment::get("G_LOG_GROUPS","") ) ;
}

G::LogOutput::LogOutput( bool enabled_and_summary , bool verbose_and_debug ,
	const std::string & stderr_replacement ) :
		m_enabled(enabled_and_summary) ,
		m_summary_log(enabled_and_summary) ,
		m_verbose_log(verbose_and_debug) ,
		m_quiet(false) ,
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
	if( pthis() == nullptr )
		pthis() = this ;
	init() ;
	groups( G::Environment::get("G_LOG_GROUPS","") ) ;
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
		pthis() = nullptr ;
	cleanup() ;
}

G::LogOutput * & G::LogOutput::pthis()
{
	static G::LogOutput * p = nullptr ;
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

void G::LogOutput::quiet( bool quiet )
{
	m_quiet = quiet ; // used in o/s-specific rawOutput
}

void G::LogOutput::verbose( bool verbose_log )
{
	m_verbose_log = verbose_log ;
}

void G::LogOutput::output( Log::Severity severity , const char * file , int line , const std::string & text )
{
	if( instance() != nullptr )
		instance()->doOutput( severity , file , line , text ) ;
}

bool G::LogOutput::at( Log::Severity severity ) const
{
	bool do_output = m_enabled ;
	if( severity == Log::s_Debug )
		do_output = m_enabled && m_debug ;
	else if( severity == Log::s_LogSummary )
		do_output = m_enabled && m_summary_log ;
	else if( severity == Log::s_LogVerbose )
		do_output = m_enabled && m_verbose_log ;
	return do_output ;
}

void G::LogOutput::groups( const std::string & list )
{
	if( instance() )
	{
		instance()->m_groups.clear() ;
		const char sep = ',' ;
		size_t p1 = 0U ;
		const size_t npos = std::string::npos ;
		for( size_t p2 = list.find(sep,p1) ; p1 < list.size() ; p1 = p2==npos?npos:(p2+1U) , p2 = list.find(sep,p1) )
		{
			if( p1 != p2 )
				instance()->m_groups.insert( list.substr( p1 , p2==npos ? p2 : (p2-p1) ) ) ;
		}
	}
}

bool G::LogOutput::at( Log::Severity severity , const std::string & group ) const
{
	if( m_groups.empty() )
		return at( severity ) ;
	else
		return at(severity) && ( m_groups.find("log-all") != m_groups.end() || m_groups.find("log-"+group) != m_groups.end() ) ;
}

void G::LogOutput::doOutput( Log::Severity severity , const char * /*file*/ , int /*line*/ , const std::string & text )
{
	bool do_output = at( severity ) ;
	if( do_output )
	{
		// reserve the buffer
		const size_type limit = static_cast<size_type>(limits::log) ;
		m_buffer.reserve( (text.length()>limit?limit:text.length()) + 40U ) ;
		m_buffer.clear() ;

		// add the preamble to the buffer
		std::string::size_type text_pos = 0U ;
		if( m_prefix.length() )
		{
			m_buffer.append( m_prefix ) ;
			m_buffer.append( ": " ) ;
		}
		if( m_timestamp )
			appendTimestampStringTo( m_buffer ) ;
		if( m_level )
			m_buffer.append( levelString(severity) ) ;

		// strip the first word from the text - expected to be the method name
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
		m_buffer.append( text , text_pos , text_len ) ;
		if( limited )
			m_buffer.append( " ..." ) ;

		// last ditch removal of ansi escape sequences
		while( m_buffer.find('\033') != std::string::npos )
			m_buffer[m_buffer.find('\033')] = '.' ;

		// do the actual output in an o/s-specific manner
		rawOutput( m_std_err , severity , m_buffer ) ;
	}
}

void G::LogOutput::onAssert()
{
	// no-op
}

void G::LogOutput::appendTimestampStringTo( std::string & result )
{
	// use a cache to optimise away calls to localtime() and strftime()
	G::EpochTime now = G::DateTime::now() ;
	if( m_time == 0 || m_time != now.s )
	{
		m_time = now.s ;
		static struct std::tm zero_broken_down_time ;
		struct std::tm broken_down_time = zero_broken_down_time ;
		getLocalTime( m_time , &broken_down_time ) ;
		m_time_buffer.reserve( 30U ) ;
		m_time_buffer.assign( 17U , '\0' ) ;
		std::strftime( &m_time_buffer[0] , m_time_buffer.size() , "%Y" "%m" "%d." "%H" "%M" "%S.", &broken_down_time ) ;
		m_time_buffer.pop_back() ;
	}

	result.append( &m_time_buffer[0] , m_time_buffer.size() ) ;
	result.append( 1U , '0' + ( ( now.us / 100000 ) % 10 ) ) ;
	result.append( 1U , '0' + ( ( now.us / 10000 ) % 10 ) ) ;
	result.append( 1U , '0' + ( ( now.us / 1000 ) % 10 ) ) ;
	result.append( ": " ) ;
}

std::string G::LogOutput::dateString()
{
	// (prefer to not use G::Date here to avoid reentrancy)
	static struct std::tm zero_broken_down_time ;
	struct std::tm broken_down_time = zero_broken_down_time ;
	getLocalTime( std::time(nullptr) , &broken_down_time ) ;
	char buffer[10] = { 0 } ;
	std::strftime( buffer , sizeof(buffer)-1U , "%Y" "%m" "%d" , &broken_down_time ) ;
	buffer[sizeof(buffer)-1U] = '\0' ;
	return std::string( buffer ) ;
}

std::string G::LogOutput::fileAndLine( const char * file , int line )
{
	if( file != nullptr )
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

void G::LogOutput::assertion( const char * file , int line , bool test , const char * test_string )
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

bool G::LogOutput::syslog() const
{
	return m_syslog ;
}
/// \file glogoutput.cpp
