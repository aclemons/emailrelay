//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file glogoutput.cpp
///

#include "gdef.h"
#include "glogoutput.h"
#include "gdatetime.h"
#include "gscope.h"
#include "gfile.h"
#include "ggettext.h"
#include "gomembuf.h"
#include "gstringview.h"
#include "glimits.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>
#include <cstring>
#include <array>

namespace G
{
	namespace LogOutputImp
	{
		constexpr int stderr_fileno = 2 ; // STDERR_FILENO
		LogOutput * this_ = nullptr ;
		constexpr std::size_t margin = 7U ;
		constexpr std::size_t buffer_base_size = Limits<>::log + 40U ;
		std::array<char,buffer_base_size+margin> buffer_1 {} ;
		std::array<char,8> buffer_2 {} ;
		struct ostream : std::ostream
		{
			explicit ostream( G::omembuf * p ) : std::ostream(p) {}
			void reset() { clear() ; seekp(0) ; }
		} ;
		std::ostream & ostream1()
		{
			static G::omembuf buf( &buffer_1[0] , buffer_1.size() ) ; // bogus clang-tidy cert-err58-cpp
			static ostream s( &buf ) ;
			s.reset() ;
			return s ;
		}
		std::ostream & ostream2()
		{
			// an ostream for junk that gets discarded
			static G::omembuf buf( &buffer_2[0] , buffer_2.size() ) ;
			static ostream s( &buf ) ;
			s.reset() ;
			return s ;
		}
		std::size_t tellp( std::ostream & s )
		{
			s.clear() ;
			return static_cast<std::size_t>( std::max( std::streampos(0) , s.tellp() ) ) ;
		}
		G::string_view info()
		{
			static std::string s( txt("info: ") ) ;
			return {s.data(),s.size()} ;
		}
		G::string_view warning()
		{
			static std::string s( txt("warning: ") ) ;
			return {s.data(),s.size()} ;
		}
		G::string_view error()
		{
			static std::string s( txt("error: ") ) ;
			return {s.data(),s.size()} ;
		}
		G::string_view fatal()
		{
			static std::string s( txt("fatal: ") ) ;
			return {s.data(),s.size()} ;
		}
	}
}

G::LogOutput::LogOutput( const std::string & exename , const Config & config ,
	const std::string & path ) :
		m_exename(exename) ,
		m_config(config) ,
		m_path(path)
{
	updateTime() ;
	open( m_path , true ) ;
	osinit() ;
	if( LogOutputImp::this_ == nullptr )
		LogOutputImp::this_ = this ;
}

G::LogOutput::LogOutput( bool output_enabled_and_summary_info ,
	bool verbose_info_and_debug , const std::string & path ) :
		m_path(path)
{
	m_config = Config()
		.set_output_enabled(output_enabled_and_summary_info)
		.set_summary_info(output_enabled_and_summary_info)
		.set_verbose_info(verbose_info_and_debug)
		.set_debug(verbose_info_and_debug) ;

	updateTime() ;
	open( m_path , true ) ;
	osinit() ;
	if( LogOutputImp::this_ == nullptr )
		LogOutputImp::this_ = this ;
}

G::LogOutput::Config G::LogOutput::config() const
{
	return m_config ;
}

void G::LogOutput::configure( const Config & config )
{
	m_config = config ;
}

G::LogOutput::~LogOutput()
{
	if( LogOutputImp::this_ == this )
		LogOutputImp::this_ = nullptr ;
	if( !m_path.empty() && m_fd != LogOutputImp::stderr_fileno && m_fd >= 0 )
		G::File::close( m_fd ) ;
	oscleanup() ;
}

G::LogOutput * G::LogOutput::instance() noexcept
{
	return LogOutputImp::this_ ;
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

bool G::LogOutput::at( Log::Severity severity ) const noexcept
{
	bool do_output = m_config.m_output_enabled ;
	if( severity == Log::Severity::Debug )
		do_output = m_config.m_output_enabled && m_config.m_debug ;
	else if( severity == Log::Severity::InfoSummary )
		do_output = m_config.m_output_enabled && m_config.m_summary_info ;
	else if( severity == Log::Severity::InfoVerbose )
		do_output = m_config.m_output_enabled && m_config.m_verbose_info ;
	return do_output ;
}

std::ostream & G::LogOutput::start( Log::Severity severity , const char * , int )
{
	if( instance() )
		return instance()->start( severity ) ;
	else
		return LogOutputImp::ostream2() ;
}

void G::LogOutput::output( std::ostream & ss )
{
	if( instance() )
		instance()->output( ss , 0 ) ;
}

void G::LogOutput::open( std::string path , bool do_throw )
{
	if( path.empty() )
	{
		m_fd = LogOutputImp::stderr_fileno ;
	}
	else
	{
		std::size_t pos = path.find( "%d" ) ;
		if( pos != std::string::npos )
			path.replace( pos , 2U , std::string(&m_time_buffer[0],8U) ) ;

		int fd = File::open( path.c_str() , File::InOutAppend::Append ) ;
		if( fd < 0 )
		{
			if( do_throw )
				throw LogFileError( path ) ;
		}
		else
		{
			if( m_fd >= 0 && m_fd != LogOutputImp::stderr_fileno )
				G::File::close( m_fd ) ;
			m_fd = fd ;
		}
	}
}

std::ostream & G::LogOutput::start( Log::Severity severity )
{
	m_depth++ ;
	if( m_depth > 1 )
		return LogOutputImp::ostream2() ;

	if( updateTime() )
		open( m_path , false ) ;

	std::ostream & ss = LogOutputImp::ostream1() ;
	ss << std::dec ;
	if( m_exename.length() )
		ss << m_exename << ": " ;
	if( m_config.m_with_timestamp )
		appendTimeTo( ss ) ;
	if( m_config.m_with_level )
		ss << levelString( severity ) ;
	if( m_config.m_with_context && m_context_fn )
		ss << (*m_context_fn)( m_context_fn_arg ) ;

	m_start_pos = LogOutputImp::tellp( ss ) ;
	m_severity = severity ;
	return ss ;
}

void G::LogOutput::output( std::ostream & ss , int )
{
	// reject nested logging
	if( m_depth ) m_depth-- ;
	if( m_depth ) return ;

	char * buffer = &LogOutputImp::buffer_1[0] ;
	std::size_t n = LogOutputImp::tellp( ss ) ;

	// elipsis on overflow
	if( n >= LogOutputImp::buffer_base_size )
	{
		char * margin = buffer + LogOutputImp::buffer_base_size ;
		margin[0] = ' ' ;
		margin[1] = '.' ;
		margin[2] = '.' ;
		margin[3] = '.' ;
		n = LogOutputImp::buffer_base_size + 4U ;
	}

	// strip the first word from the text - expected to be the method name
	char * p = buffer ;
	if( m_config.m_strip )
	{
		char * end = buffer + n ;
		char * text = buffer + m_start_pos ;
		char * space = std::find( text , end , ' ' ) ;
		if( space != end && (space+1) != end )
		{
			// (move the preamble forwards)
			p = std::copy_backward( buffer , text , space+1 ) ;
			n -= (p-buffer) ;
		}
	}

	// last-ditch removal of ansi escape sequences
	p[n] = '\0' ;
	for( char * pp = std::strchr(buffer,'\033') ; pp ; pp = std::strchr(p+1,'\033') )
		*pp = '.' ;

	// do the actual output in an o/s-specific manner -- the margin
	// allows the implementation to extend the text with eg. a newline
	osoutput( m_fd , m_severity , p , n ) ;
}

void G::LogOutput::assertionFailure( const char * file , int line , const char * test_expression ) noexcept
{
	// ('noexcept' on this fn so we std::terminate() if any of this throws)
	if( instance() )
	{
		std::ostream & ss = LogOutputImp::ostream1() ;
		ss << txt("assertion error: ") << basename(file) << "(" << line << "): " << test_expression ;
		char * p = &LogOutputImp::buffer_1[0] ;
		std::size_t n = LogOutputImp::tellp( ss ) ;
		instance()->osoutput( instance()->m_fd , Log::Severity::Assertion , p , n ) ;
	}
	else
	{
		std::cerr << txt("assertion error: ") << basename(file) << "(" << line << "): " << test_expression << std::endl ;
	}
}

void G::LogOutput::assertionAbort()
{
	std::abort() ;
}

bool G::LogOutput::updateTime()
{
	SystemTime now = SystemTime::now() ;
	m_time_us = now.us() ;
	bool new_day = false ;
	if( m_time_s == 0 || m_time_s != now.s() || m_time_buffer.empty() )
	{
		m_time_s = now.s() ;
		m_time_buffer.resize( 17U ) ;
		now.local().format( m_time_buffer , "%Y%m%d.%H%M%S." ) ;
		m_time_buffer[16U] = '\0' ;
		new_day = 0 != std::memcmp( &m_date_buffer[0] , &m_time_buffer[0] , m_date_buffer.size() ) ;
		std::memcpy( &m_date_buffer[0] , &m_time_buffer[0] , m_date_buffer.size() ) ;
	}
	return new_day ;
}

void G::LogOutput::appendTimeTo( std::ostream & ss )
{
	ss
		<< &m_time_buffer[0]
		<< static_cast<char>( '0' + ( ( m_time_us / 100000U ) % 10U ) )
		<< static_cast<char>( '0' + ( ( m_time_us / 10000U ) % 10U ) )
		<< static_cast<char>( '0' + ( ( m_time_us / 1000U ) % 10U ) )
		<< ": " ;
}

const char * G::LogOutput::basename( const char * file ) noexcept
{
	if( file == nullptr ) return "" ;
	const char * p1 = std::strrchr( file , '/' ) ;
	const char * p2 = std::strrchr( file , '\\' ) ;
	return p1 > p2 ? (p1+1) : (p2?(p2+1):file) ;
}

G::string_view G::LogOutput::levelString( Log::Severity s ) noexcept
{
	namespace imp = LogOutputImp ;
	if( s == Log::Severity::Debug ) return "debug: "_sv ;
	else if( s == Log::Severity::InfoSummary ) return imp::info() ;
	else if( s == Log::Severity::InfoVerbose ) return imp::info() ;
	else if( s == Log::Severity::Warning ) return imp::warning() ;
	else if( s == Log::Severity::Error ) return imp::error() ;
	else if( s == Log::Severity::Assertion ) return imp::fatal() ;
	return "" ;
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

G::LogOutput::Config & G::LogOutput::Config::set_allow_bad_syslog( bool value )
{
	m_allow_bad_syslog = value ;
	return *this ;
}

G::LogOutput::Config & G::LogOutput::Config::set_facility( SyslogFacility facility )
{
	m_facility = facility ;
	return *this ;
}

