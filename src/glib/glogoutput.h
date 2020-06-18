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
///
/// \file glogoutput.h
///

#ifndef G_LOG_OUTPUT_H
#define G_LOG_OUTPUT_H

#include "gdef.h"
#include "glog.h"
#include <string>
#include <vector>
#include <fstream>
#include <ctime>

namespace G
{
	class LogOutput ;
}

/// \class G::LogOutput
/// Controls and implements low-level logging output, as used by the Log
/// interface. Applications should instantiate a LogOutput object in main() to
/// enable log output.
/// \see G::Log
///
class G::LogOutput
{
public:
 	enum class SyslogFacility { User , Daemon , Mail , Cron } ; // etc.

	struct Config /// A configuration structure for G::LogOutput.
	{
		bool m_output_enabled{false} ;
		bool m_summary_info{false} ;
		bool m_verbose_info{false} ;
		bool m_debug{false} ;
		bool m_with_level{false} ;
		bool m_with_timestamp{false} ;
		bool m_with_context{false} ;
		bool m_strip{false} ; // first word
		bool m_quiet_stderr{false} ;
		bool m_use_syslog{false} ;
		Config() ;
		Config & set_output_enabled( bool value = true ) ;
		Config & set_summary_info( bool value = true ) ;
		Config & set_verbose_info( bool value = true ) ;
		Config & set_debug( bool value = true ) ;
		Config & set_with_level( bool value = true ) ;
		Config & set_with_timestamp( bool value = true ) ;
		Config & set_with_context( bool value = true ) ;
		Config & set_strip( bool value = true ) ;
		Config & set_quiet_stderr( bool value = true ) ;
		Config & set_use_syslog( bool value = true ) ;
	} ;

	LogOutput( const std::string & exename , const Config & config ,
		const std::string & stderr_replacement = std::string() ,
		SyslogFacility syslog_facility = SyslogFacility::User ) ;
			///< Constructor. If there is no LogOutput object, or if
			///< 'config.output_enabled' is false, then there is no
			///< output of any sort. Otherwise at least warning and
			///< error messages are generated.
			///<
			///< If 'config.summary_info' is true then log-summary
			///< messages are output. If 'config.verbose_info' is true
			///< then log-verbose messages are output. If 'config.with_debug'
			///< is true then debug messages will also be generated (but
			///< only if compiled in).
			///<
			///< More than one LogOutput object may be created, but only
			///< the first one controls output.

	explicit LogOutput( bool output_enabled_and_summary_info ,
		bool verbose_info_and_debug = true ,
		const std::string & stderr_replacement = std::string() ) ;
			///< Constructor for test programs. Only generates output if the
			///< first parameter is true. Never uses syslog.

	~LogOutput() ;
		///< Destructor.

	static LogOutput * instance() noexcept ;
		///< Returns a pointer to the controlling LogOutput object. Returns
		///< nullptr if none.

	Config config() const ;
		///< Returns the current configuration.

	void configure( const Config & ) ;
		///< Updates the current configuration.

	bool at( Log::Severity ) const ;
		///< Returns true if logging should occur for the given severity level.

	static void context( std::string (*fn)(void*) = nullptr , void * fn_arg = nullptr ) noexcept ;
		///< Sets a functor that is used to provide a context string for
		///< every log line, if configured. The functor should return
		///< the context string with trailing punctuation, typically
		///< colon and space.

	static void * contextarg() noexcept ;
		///< Returns the functor argument as set by the last call to context().

	static void output( Log::Severity , const char * file , int line , const std::string & ) ;
		///< Generates output if there is an existing LogOutput object
		///< which is enabled. Uses rawOutput().

	static void assertion( const char * file , int line , bool test , const char * test_string ) ;
		///< Performs an assertion check.

	static void assertion( const char * file , int line , void * test , const char * test_string ) ;
		///< Performs an assertion check. This overload, using a test on a pointer,
		///< is motivated by MSVC warnings.

	static void assertionFailure( const char * file , int line , const char * test_expression ) noexcept ;
		///< Reports an assertion failure.

	static void assertionAbort() G_ANALYZER_NORETURN ;
		///< Aborts the program when an assertion has failed.

	static void register_( const std::string & exe ) ;
		///< Registers the given executable as a source of logging.
		///< This is called from init(), but it might also need to be
		///< done as an installation task with the necessary
		///< permissions.

public:
	LogOutput( const LogOutput & ) = delete ;
	LogOutput( LogOutput && ) = delete ;
	void operator=( const LogOutput & ) = delete ;
	void operator=( LogOutput && ) = delete ;

private:
	void init() ;
	static void open( std::ofstream & , const std::string & ) ;
	void cleanup() const noexcept ;
	void appendTimestampStringTo( std::string & ) ;
	void doOutput( Log::Severity , const std::string & ) ;
	void doOutput( Log::Severity s , const char * , int , const std::string & ) ;
	void rawOutput( std::ostream & , Log::Severity , const std::string & ) ;
	static const char * levelString( Log::Severity s ) ;
	static std::string itoa( int ) ;
	static std::string fileAndLine( const char * , int ) ;
	static LogOutput * & pthisref() noexcept ;
	static std::ostream & err( const std::string & ) ;
	static void assertionFailure( ) ;

private:
	std::string m_exename ;
	Config m_config ;
	std::ostream & m_std_err ;
	SyslogFacility m_facility ;
	std::time_t m_time{0} ;
	std::vector<char> m_time_buffer ;
	bool m_timestamp{false} ;
	HANDLE m_handle{0} ; // windows
	bool m_handle_set{false} ;
	std::string m_buffer ;
	bool m_in_output{false} ;
	std::string (*m_context_fn)(void *) ;
	void * m_context_fn_arg{nullptr} ;
} ;

inline void G::LogOutput::assertion( const char * file , int line , bool test , const char * test_string )
{
	if( !test )
	{
		assertionFailure( file , line , test_string ) ;
		assertionAbort() ;
	}
}

inline void G::LogOutput::assertion( const char * file , int line , void * test , const char * test_string )
{
	if( !test )
	{
		assertionFailure( file , line , test_string ) ;
		assertionAbort() ;
	}
}

#endif
