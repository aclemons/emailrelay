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
/// \file glogoutput.cpp
///

#include "gdef.h"
#include "glogoutput.h"
#include "gdatetime.h"
#include "gscope.h"
#include "gfile.h"
#include "ggettext.h"
#include "gstringview.h"
#include "groot.h"
#include "gtest.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <cstring>
#include <array>

G_LOG_THREAD_LOCAL G::LogOutput * G::LogOutput::m_instance = nullptr ;

namespace G
{
	namespace LogOutputImp
	{
		constexpr int stdout_fileno = 1 ; // STDOUT_FILENO
		constexpr int stderr_fileno = 2 ; // STDERR_FILENO
		std::size_t tellp( LogStream & log_stream )
		{
			if( log_stream.m_ostream == nullptr ) return 0U ;
			log_stream.m_ostream->clear() ;
			return static_cast<std::size_t>( std::max( std::streampos(0) , log_stream.m_ostream->tellp() ) ) ;
		}
		std::string_view info()
		{
			static const std::string s( txt("info: ") ) ;
			return {s.data(),s.size()} ;
		}
		std::string_view warning()
		{
			static const std::string s( txt("warning: ") ) ;
			return {s.data(),s.size()} ;
		}
		std::string_view error()
		{
			static const std::string s( txt("error: ") ) ;
			return {s.data(),s.size()} ;
		}
		std::string_view assertion()
		{
			static const std::string s( txt("assertion error: ") ) ;
			return {s.data(),s.size()} ;
		}
	}
}

G::LogOutput::LogOutput( Private , const std::string & exename , const Config & config ) :
	m_exename(exename) ,
	m_config(config) ,
	m_buffer(m_buffer_size) ,
	m_streambuf(m_buffer.data(),m_buffer.size()) ,
	m_stream(&m_streambuf) ,
	m_fd(m_config.m_stdout?LogOutputImp::stdout_fileno:LogOutputImp::stderr_fileno)
{
	updateTime() ;
}

G::LogOutput::LogOutput( const std::string & exename , const Config & config , const Path & path ) :
	LogOutput({},exename,config)
{
	m_path = path ; // NOLINT initialiser list
	init() ;
}


#ifndef G_LIB_SMALL
G::LogOutput::LogOutput( const std::string & exename , const Config & config , int fd ) :
	LogOutput({},exename,config)
{
	m_fd = fd ; // NOLINT initialiser list
	init() ;
}
#endif

#ifndef G_LIB_SMALL
G::LogOutput::LogOutput( bool enabled , bool verbose , const Path & path ) :
	LogOutput({},"",{enabled,verbose})
{
	m_path = path ; // NOLINT initialiser list
	init() ;
}
#endif

void G::LogOutput::init()
{
	updatePath( m_path , m_real_path ) ;
	open( m_real_path , /*do_throw=*/true ) ;
	osinit() ;
	if( m_instance == nullptr )
		m_instance = this ;
}

G::LogOutput::Config G::LogOutput::config() const noexcept
{
	return m_config ;
}

#ifndef G_LIB_SMALL
int G::LogOutput::fd() const noexcept
{
	return m_fd ;
}
#endif

void G::LogOutput::configure( const Config & config )
{
	m_config = config ;
}

G::LogOutput::~LogOutput()
{
	static_assert( noexcept(m_path.empty()) , "" ) ;
	static_assert( noexcept(G::File::close(m_fd)) , "" ) ;
	static_assert( noexcept(oscleanup()) , "" ) ;

	if( m_instance == this )
	{
		m_instance = nullptr ;
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
	return m_instance ;
}

void G::LogOutput::context( std::string_view (*fn)(void *) , void * fn_arg ) noexcept
{
	LogOutput * p = instance() ;
	if( p )
	{
		p->m_context_fn = fn ;
		p->m_context_fn_arg = fn_arg ;
	}
}

#ifndef G_LIB_SMALL
void * G::LogOutput::contextarg() noexcept
{
	LogOutput * p = instance() ;
	return p ? p->m_context_fn_arg : nullptr ;
}
#endif

bool G::LogOutput::at( Severity severity ) const noexcept
{
	bool do_output = m_config.m_output_enabled ;
	if( severity == Severity::Debug )
		do_output = m_config.m_output_enabled && m_config.m_debug ;
	else if( severity == Severity::InfoSummary )
		do_output = m_config.m_output_enabled && m_config.m_summary_info ;
	else if( severity == Severity::InfoVerbose )
		do_output = m_config.m_output_enabled && m_config.m_verbose_info ;
	else if( severity == Severity::InfoMoreVerbose )
		do_output = m_config.m_output_enabled && m_config.m_more_verbose_info ;
	return do_output ;
}

G::LogStream G::LogOutput::start( Severity severity , const char * , int ) noexcept
{
	try
	{
		if( instance() )
			return instance()->start( severity ) ; // not noexcept
		else
			return LogStream( nullptr ) ;
	}
	catch(...)
	{
		return LogStream( nullptr ) ; // is noexcept
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

bool G::LogOutput::updatePath( const Path & path_in , Path & path_out ) const
{
	bool changed = false ;
	if( !path_in.empty() )
	{
		Path new_path_out = makePath( path_in ) ;
		changed = new_path_out != path_out ;
		path_out.swap( new_path_out ) ;
	}
	return changed ;
}

G::Path G::LogOutput::makePath( const Path & path_in ) const
{
	// this is called at most hourly (see updateTime()), so not optimised
	std::string_view yyyymmdd( m_time_buffer.data() , 8U ) ;
	std::string_view hh( m_time_buffer.data()+9 , 2U ) ;
	Path path_out = path_in ;
	path_out.replace( "%d" , yyyymmdd , /*ex_root=*/true ) ;
	path_out.replace( "%h" , hh , /*ex_root=*/true ) ;
	return path_out ;
}

void G::LogOutput::open( const Path & path , bool do_throw )
{
	if( !path.empty() )
	{
		int fd = -1 ;
		{
			Process::Umask set_umask( m_config.m_umask ) ;
			Root claim_root ;
			fd = File::open( path , File::InOutAppend::Append ) ;
			if( fd < 0 && do_throw )
				throw LogFileError( path.str() ) ;
		}
		if( fd >= 0 )
		{
			if( m_fd >= 0 && m_fd != LogOutputImp::stderr_fileno && m_fd != LogOutputImp::stdout_fileno )
				G::File::close( m_fd ) ;
			m_fd = fd ;
		}
	}
}

G::LogStream G::LogOutput::start( Severity severity )
{
	m_depth++ ;
	if( m_depth > 1 )
		return LogStream( nullptr ) ;

	if( updateTime() && updatePath(m_path,m_real_path) )
		open( m_real_path , false ) ;

	m_stream.reset() ;
	LogStream log_stream( &m_stream ) ;

	log_stream << std::dec ;
	if( !m_exename.empty() )
		log_stream << m_exename << ": " ;
	if( m_config.m_with_timestamp )
		appendTimeTo( log_stream ) ;
	if( m_config.m_with_level )
		log_stream << levelString( severity ) ;
	if( m_config.m_with_context && m_context_fn )
		log_stream << (*m_context_fn)( m_context_fn_arg ) ;

	m_start_pos = LogOutputImp::tellp( log_stream ) ;
	m_severity = severity ;
	return log_stream ;
}

void G::LogOutput::output( LogStream & log_stream , int )
{
	// reject nested logging
	if( m_depth ) m_depth-- ;
	if( m_depth ) return ;

	char * buffer = m_buffer.data() ;
	std::size_t n = LogOutputImp::tellp( log_stream ) ;

	// elipsis on overflow
	if( n >= m_buffer_base_size )
	{
		static_assert( m_rhs_margin > 4U , "" ) ;
		char * margin = buffer + m_buffer_base_size ;
		margin[0] = ' ' ;
		margin[1] = '.' ;
		margin[2] = '.' ;
		margin[3] = '.' ;
		n = m_buffer_base_size + 4U ;
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

void G::LogOutput::assertionFailure( LogOutput * instance , const char * file , int line , const char * test_expression ) noexcept
{
	// ('noexcept' on this fn so we std::terminate() if any of this throws)
	if( instance )
	{
		// (not strictly thread-safe, but we are std::abort()ing anyway)
		static std::array<char,m_buffer_size> buffer ;
		omembuf streambuf( buffer.data() , buffer.size() ) ;
		Stream stream( &streambuf ) ;
		LogStream log_stream( &stream ) ;

		log_stream << LogOutputImp::assertion() << basename(file) << "(" << line << "): " << test_expression ;
		char * p = buffer.data() ;
		std::size_t n = std::min( std::size_t(m_buffer_base_size) , LogOutputImp::tellp(log_stream) ) ; // (size_t cast is gcc workround)
		instance->osoutput( instance->m_fd , Severity::Assertion , p , n ) ;
	}
	else
	{
		std::cerr << LogOutputImp::assertion() << basename(file) << "(" << line << "): " << test_expression << std::endl ;
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

void G::LogOutput::appendTimeTo( LogStream & log_stream )
{
	log_stream
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

std::string_view G::LogOutput::levelString( Severity s ) noexcept
{
	namespace imp = LogOutputImp ;
	if( s == Severity::Debug ) return "debug: " ;
	else if( s == Severity::InfoSummary ) return imp::info() ;
	else if( s == Severity::InfoVerbose ) return imp::info() ;
	else if( s == Severity::InfoMoreVerbose ) return imp::info() ;
	else if( s == Severity::Warning ) return imp::warning() ;
	else if( s == Severity::Error ) return imp::error() ;
	else if( s == Severity::Assertion ) return imp::assertion() ;
	return "" ;
}

// ==

G::LogOutput::Config::Config() noexcept
= default;

G::LogOutput::Config::Config( bool enabled , bool verbose ) noexcept :
	m_output_enabled(enabled),
	m_summary_info(enabled),
	m_verbose_info(verbose),
	m_more_verbose_info(verbose),
	m_debug(verbose)
{
}

