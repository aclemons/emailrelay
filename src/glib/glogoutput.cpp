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
#include "groot.h"
#include "gtest.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <cstring>
#include <array>

namespace G
{
	namespace LogOutputImp
	{
		constexpr int stdout_fileno = 1 ; // STDOUT_FILENO
		constexpr int stderr_fileno = 2 ; // STDERR_FILENO
		LogOutput * this_ = nullptr ;
		constexpr std::size_t margin = 7U ;
		constexpr std::size_t buffer_base_size = Limits<>::log + 40U ;
		std::array<char,buffer_base_size+margin> buffer {} ;
		struct ostream : std::ostream /// An ostream using G::omembuf.
		{
			explicit ostream( G::omembuf * p ) : std::ostream(p) {}
			void reset() { clear() ; seekp(0) ; }
		} ;
		LogStream & ostream1()
		{
			static G::omembuf buf( buffer.data() , buffer.size() ) ; // bogus clang-tidy cert-err58-cpp
			static ostream s( &buf ) ;
			static LogStream logstream( &s ) ;
			s.reset() ;
			return logstream ;
		}
		LogStream & ostream2() noexcept
		{
			// an ostream for junk that gets discarded
			static LogStream logstream( nullptr ) ; // is noexcept
			return logstream ;
		}
		std::size_t tellp( LogStream & logstream )
		{
			if( !logstream.m_ostream ) return 0U ;
			logstream.m_ostream->clear() ;
			return static_cast<std::size_t>( std::max( std::streampos(0) , logstream.m_ostream->tellp() ) ) ;
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
	updatePath( m_path , m_real_path ) ;
	open( m_real_path , true ) ;
	osinit() ;
	if( LogOutputImp::this_ == nullptr )
		LogOutputImp::this_ = this ;
}

#ifndef G_LIB_SMALL
G::LogOutput::LogOutput( bool output_enabled_and_summary_info ,
	bool verbose_info_and_debug , const std::string & path ) :
		m_path(path)
{
	m_config = Config()
		.set_output_enabled(output_enabled_and_summary_info)
		.set_summary_info(output_enabled_and_summary_info)
		.set_verbose_info(verbose_info_and_debug)
		.set_more_verbose_info(verbose_info_and_debug)
		.set_debug(verbose_info_and_debug) ;

	updateTime() ;
	updatePath( m_path , m_real_path ) ;
	open( m_real_path , true ) ;
	osinit() ;
	if( LogOutputImp::this_ == nullptr )
		LogOutputImp::this_ = this ;
}
#endif

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
	{
		LogOutputImp::this_ = nullptr ;
	}
	if( !m_path.empty() && m_fd >= 0 &&
		m_fd != LogOutputImp::stderr_fileno &&
		m_fd != LogOutputImp::stdout_fileno )
	{
		G::File::close( m_fd ) ;
	}
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
	else if( severity == Log::Severity::InfoMoreVerbose )
		do_output = m_config.m_output_enabled && m_config.m_more_verbose_info ;
	return do_output ;
}

G::LogStream & G::LogOutput::start( Log::Severity severity , const char * , int ) noexcept
{
	try
	{
		if( instance() )
			return instance()->start( severity ) ; // not noexcept
		else
			return LogOutputImp::ostream2() ;
	}
	catch(...)
	{
		return LogOutputImp::ostream2() ; // is noexcept
	}
}

void G::LogOutput::output( LogStream & s ) noexcept
{
	try
	{
		if( instance() )
			instance()->output( s , 0 ) ; // not noexcept
	}
	catch(...)
	{
	}
}

bool G::LogOutput::updatePath( const std::string & path_in , std::string & path_out ) const
{
	bool changed = false ;
	if( !path_in.empty() )
	{
		std::string new_path_out = makePath( path_in ) ;
		changed = new_path_out != path_out ;
		path_out.swap( new_path_out ) ;
	}
	return changed ;
}

std::string G::LogOutput::makePath( const std::string & path_in ) const
{
	// this is called at most hourly, so not optimised
	std::string path_out = path_in ;
	std::size_t pos = 0U ;
	if( (pos=path_out.find("%d")) != std::string::npos ) // NOLINT assignment
	{
		std::string yyyymmdd( m_time_buffer.data() , 8U ) ;
		path_out.replace( pos , 2U , yyyymmdd ) ;
	}
	if( (pos=path_out.find("%h")) != std::string::npos ) // NOLINT assignment
	{
		path_out[pos] = m_time_buffer[9] ;
		path_out[pos+1U] = m_time_buffer[10] ;
	}
	return path_out ;
}

void G::LogOutput::open( const std::string & path , bool do_throw )
{
	if( path.empty() )
	{
		m_fd = m_config.m_stdout ? LogOutputImp::stdout_fileno : LogOutputImp::stderr_fileno ;
	}
	else
	{
		int fd = -1 ;
		{
			Process::Umask set_umask( m_config.m_umask ) ;
			Root claim_root ;
			fd = File::open( path.c_str() , File::InOutAppend::Append ) ;
			if( fd < 0 && do_throw )
				throw LogFileError( path ) ;
		}
		if( fd >= 0 )
		{
			if( m_fd >= 0 && m_fd != LogOutputImp::stderr_fileno && m_fd != LogOutputImp::stdout_fileno )
				G::File::close( m_fd ) ;
			m_fd = fd ;
		}
	}
}

G::LogStream & G::LogOutput::start( Log::Severity severity )
{
	m_depth++ ;
	if( m_depth > 1 )
		return LogOutputImp::ostream2() ;

	if( updateTime() && updatePath(m_path,m_real_path) )
		open( m_real_path , false ) ;

	LogStream & logstream = LogOutputImp::ostream1() ;
	logstream << std::dec ;
	if( m_exename.length() )
		logstream << m_exename << ": " ;
	if( m_config.m_with_timestamp )
		appendTimeTo( logstream ) ;
	if( m_config.m_with_level )
		logstream << levelString( severity ) ;
	if( m_config.m_with_context && m_context_fn )
		logstream << (*m_context_fn)( m_context_fn_arg ) ;

	m_start_pos = LogOutputImp::tellp( logstream ) ;
	m_severity = severity ;
	return logstream ;
}

void G::LogOutput::output( LogStream & logstream , int )
{
	// reject nested logging
	if( m_depth ) m_depth-- ;
	if( m_depth ) return ;

	char * buffer = LogOutputImp::buffer.data() ;
	std::size_t n = LogOutputImp::tellp( logstream ) ;

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

	if( m_fd == LogOutputImp::stdout_fileno )
		std::cout.flush() ;

	// do the actual output in an o/s-specific manner -- the margin
	// allows the implementation to extend the text with eg. a newline
	osoutput( m_fd , m_severity , p , n ) ;
}

void G::LogOutput::assertionFailure( const char * file , int line , const char * test_expression ) noexcept
{
	// ('noexcept' on this fn so we std::terminate() if any of this throws)
	if( instance() )
	{
		LogStream & logstream = LogOutputImp::ostream1() ;
		logstream << txt("assertion error: ") << basename(file) << "(" << line << "): " << test_expression ;
		char * p = LogOutputImp::buffer.data() ;
		std::size_t n = LogOutputImp::tellp( logstream ) ;
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
	bool new_hour = false ;
	if( m_time_s == 0 || m_time_s != now.s() || m_time_buffer[0] == '\0' )
	{
		m_time_s = now.s() ;
		m_time_buffer[0] = '\0' ;
		now.local().format( m_time_buffer.data() , m_time_buffer.size() , "%Y%m%d.%H%M%S." ) ;
		m_time_buffer[16U] = '\0' ;

		new_hour = 0 != std::memcmp( m_time_change_buffer.data() , m_time_buffer.data() , 11U ) ;

		static_assert( sizeof(m_time_change_buffer) == sizeof(m_time_buffer) , "" ) ;
		std::memcpy( m_time_change_buffer.data() , m_time_buffer.data() , m_time_buffer.size() ) ;
	}
	return new_hour ;
}

void G::LogOutput::appendTimeTo( LogStream & logstream )
{
	logstream
		<< m_time_buffer.data()
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
	if( s == Log::Severity::Debug ) return "debug: " ;
	else if( s == Log::Severity::InfoSummary ) return imp::info() ;
	else if( s == Log::Severity::InfoVerbose ) return imp::info() ;
	else if( s == Log::Severity::InfoMoreVerbose ) return imp::info() ;
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

G::LogOutput::Config & G::LogOutput::Config::set_more_verbose_info( bool value )
{
	m_more_verbose_info = value ;
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

G::LogOutput::Config & G::LogOutput::Config::set_umask( Process::Umask::Mode umask )
{
	m_umask = umask ;
	return *this ;
}

#ifndef G_LIB_SMALL
G::LogOutput::Config & G::LogOutput::Config::set_stdout( bool value )
{
	m_stdout = value ;
	return *this ;
}
#endif

