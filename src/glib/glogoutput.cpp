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
// glogoutput.cpp
//

#include "gdef.h"
#include "gdatetime.h"
#include "glogoutput.h"
#include "genvironment.h"
#include "gscope.h"
#include "glimits.h"
#include <sstream>
#include <string>
#include <stdexcept>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <array>

G::LogOutput::LogOutput( const std::string & exename , const Config & config ,
	const std::string & stderr_replacement , SyslogFacility syslog_facility ) :
		m_exename(exename) ,
		m_config(config) ,
		m_std_err(err(stderr_replacement)) ,
		m_facility(syslog_facility) ,
		m_context_fn(nullptr)
{
	if( pthisref() == nullptr )
		pthisref() = this ;
	init() ;
}

G::LogOutput::LogOutput( bool output_enabled_and_summary_info ,
	bool verbose_info_and_debug , const std::string & stderr_replacement ) :
		m_std_err(err(stderr_replacement)) ,
		m_facility(SyslogFacility::User) ,
		m_context_fn(nullptr)
{
	m_config = Config()
		.set_output_enabled(output_enabled_and_summary_info)
		.set_summary_info(output_enabled_and_summary_info)
		.set_verbose_info(verbose_info_and_debug)
		.set_debug(verbose_info_and_debug) ;

	if( pthisref() == nullptr )
		pthisref() = this ;
	init() ;
}

G::LogOutput::Config G::LogOutput::config() const
{
	return m_config ;
}

void G::LogOutput::configure( const Config & config )
{
	m_config = config ;
}

std::ostream & G::LogOutput::err( const std::string & path_in )
{
	if( !path_in.empty() )
	{
		std::string path = path_in ;
		std::size_t pos = path.find("%d") ;
		if( pos != std::string::npos )
		{
			std::string yyyymmdd = SystemTime::now().local().str( "%Y" "%m" "%d" ) ;
			path.replace( pos , 2U , yyyymmdd ) ;
		}

		static std::ofstream file ;
		open( file , path ) ;
		if( !file.good() )
			throw std::runtime_error( "cannot open log file: " + path ) ;

		return file ;
	}
	else
	{
		return std::cerr ;
	}
}

G::LogOutput::~LogOutput()
{
	if( pthisref() == this )
		pthisref() = nullptr ;
	cleanup() ;
}

G::LogOutput * & G::LogOutput::pthisref() noexcept
{
	static LogOutput * p = nullptr ;
	return p ;
}

G::LogOutput * G::LogOutput::instance() noexcept
{
	return pthisref() ;
}

void G::LogOutput::context( std::string (*fn)(void *) , void * fn_arg ) noexcept
{
	LogOutput * p = instance() ;
	if( p )
	{
		p->m_context_fn = fn ;
		p->m_context_fn_arg = fn_arg ;
	}
}

void * G::LogOutput::contextarg() noexcept
{
	LogOutput * p = instance() ;
	return p ? p->m_context_fn_arg : nullptr ;
}

void G::LogOutput::output( Log::Severity severity , const char * file , int line , const std::string & text )
{
	if( instance() != nullptr )
		instance()->doOutput( severity , file , line , text ) ;
}

bool G::LogOutput::at( Log::Severity severity ) const
{
	bool do_output = m_config.m_output_enabled ;
	if( severity == Log::Severity::s_Debug )
		do_output = m_config.m_output_enabled && m_config.m_debug ;
	else if( severity == Log::Severity::s_InfoSummary )
		do_output = m_config.m_output_enabled && m_config.m_summary_info ;
	else if( severity == Log::Severity::s_InfoVerbose )
		do_output = m_config.m_output_enabled && m_config.m_verbose_info ;
	return do_output ;
}

void G::LogOutput::doOutput( Log::Severity severity , const char * /*file*/ , int /*line*/ , const std::string & text )
{
	if( m_in_output ) return ;
	ScopeExitSetFalse setter( m_in_output = true ) ;
	bool do_output = at( severity ) ;
	if( do_output )
	{
		// reserve the buffer
		const auto limit = static_cast<std::size_t>( limits::log ) ;
		m_buffer.reserve( (text.length()>limit?limit:text.length()) + 40U ) ;
		m_buffer.clear() ;

		// add the preamble to the buffer
		std::size_t text_pos = 0U ;
		if( m_exename.length() )
		{
			m_buffer.append( m_exename ) ;
			m_buffer.append( ": " ) ;
		}
		if( m_config.m_with_timestamp )
			appendTimestampStringTo( m_buffer ) ;
		if( m_config.m_with_level )
			m_buffer.append( levelString(severity) ) ;
		if( m_config.m_with_context && m_context_fn )
			m_buffer.append( (*m_context_fn)(m_context_fn_arg) ) ;

		// strip the first word from the text - expected to be the method name
		if( m_config.m_strip )
		{
			text_pos = text.find(' ') ;
			if( text_pos == std::string::npos || (text_pos+1U) == text.length() )
				text_pos = 0U ;
			else
				text_pos++ ;
		}

		// add the text to the buffer, with a sanity limit
		std::size_t text_len = text.length() - text_pos ;
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

void G::LogOutput::appendTimestampStringTo( std::string & result )
{
	// use a cache to optimise away calls to localtime() and strftime()
	SystemTime now = SystemTime::now() ;
	if( m_time == 0 || m_time != now.s() || m_time_buffer.empty() )
	{
		m_time = now.s() ;
		m_time_buffer.resize( 17U ) ;
		now.local().format( m_time_buffer , "%Y" "%m" "%d." "%H" "%M" "%S." ) ;
		m_time_buffer[m_time_buffer.size()-1U] = '\0' ; // just in case
	}

	result.append( &m_time_buffer[0] ) ;
	result.append( 1U , '0' + ( ( now.us() / 100000 ) % 10 ) ) ;
	result.append( 1U , '0' + ( ( now.us() / 10000 ) % 10 ) ) ;
	result.append( 1U , '0' + ( ( now.us() / 1000 ) % 10 ) ) ;
	result.append( ": " ) ;
}

std::string G::LogOutput::fileAndLine( const char * file , int line )
{
	if( file != nullptr )
	{
		std::string basename( file ) ;
		std::size_t slash_pos = basename.find_last_of( "/\\" ) ;
		if( slash_pos != std::string::npos && (slash_pos+1U) < basename.length() )
			basename.erase( 0U , slash_pos+1U ) ;
		return basename + "(" + itoa(line) + "): " ;
	}
	else
	{
		return std::string() ;
	}
}

void G::LogOutput::assertionFailure( const char * file , int line , const char * test_expression ) noexcept
{
	// (the 'noexcept' on this fn means that we std::terminate() if any of this throws)
	if( instance() )
	{
		instance()->rawOutput( instance()->m_std_err , Log::Severity::s_Assertion ,
			std::string() + "Assertion error: " + fileAndLine(file,line) + test_expression ) ;
	}
	else
	{
		std::cerr << "assertion error: " << file << "(" << line << "): " << test_expression << std::endl ;
	}
}

void G::LogOutput::assertionAbort() // [[analyzer_noreturn]]
{
	std::abort() ;
}

const char * G::LogOutput::levelString( Log::Severity s )
{
	if( s == Log::Severity::s_Debug ) return "debug: " ;
	else if( s == Log::Severity::s_InfoSummary ) return "info: " ;
	else if( s == Log::Severity::s_InfoVerbose ) return "info: " ;
	else if( s == Log::Severity::s_Warning ) return "warning: " ;
	else if( s == Log::Severity::s_Error ) return "error: " ;
	else if( s == Log::Severity::s_Assertion ) return "fatal: " ;
	return "" ;
}

std::string G::LogOutput::itoa( int n_in )
{
	// diy implementation for speed and portability
	if( n_in < 0 ) return std::string(1U,'0') ;
	auto n = static_cast<unsigned int>(n_in) ;
	std::array<char,20U> buffer {{ '0' , '\0' }} ;
	const unsigned int million = 1000000U ;
	if( n > million ) n %= million ;
	bool zero = n == 0U ;
	char * p = &buffer[0] + buffer.size() - 1U ;
	for( *p-- = '\0' ; n > 0U ; --p , n /= 10U )
		*p = static_cast<char>( '0' + (n % 10U) ) ;
	return zero ? std::string(1U,'0') : std::string(p+1U) ;
}

// ==

G::LogOutput::Config::Config()
= default;

G::LogOutput::Config & G::LogOutput::Config::set_output_enabled( bool value )
{
	m_output_enabled = value ;
	return *this ;
}

G::LogOutput::Config & G::LogOutput::Config::set_summary_info( bool value )
{
	m_summary_info = value ;
	return *this ;
}

G::LogOutput::Config & G::LogOutput::Config::set_verbose_info( bool value )
{
	m_verbose_info = value ;
	return *this ;
}

G::LogOutput::Config & G::LogOutput::Config::set_debug( bool value )
{
	m_debug = value ;
	return *this ;
}

G::LogOutput::Config & G::LogOutput::Config::set_with_level( bool value )
{
	m_with_level = value ;
	return *this ;
}

G::LogOutput::Config & G::LogOutput::Config::set_with_timestamp( bool value )
{
	m_with_timestamp = value ;
	return *this ;
}

G::LogOutput::Config & G::LogOutput::Config::set_with_context( bool value )
{
	m_with_context = value ;
	return *this ;
}

G::LogOutput::Config & G::LogOutput::Config::set_strip( bool value )
{
	m_strip = value ;
	return *this ;
}

G::LogOutput::Config & G::LogOutput::Config::set_quiet_stderr( bool value )
{
	m_quiet_stderr = value ;
	return *this ;
}

G::LogOutput::Config & G::LogOutput::Config::set_use_syslog( bool value )
{
	m_use_syslog = value ;
	return *this ;
}

/// \file glogoutput.cpp
