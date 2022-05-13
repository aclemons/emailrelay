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
/// \file glogoutput.h
///

#ifndef G_LOG_OUTPUT_H
#define G_LOG_OUTPUT_H

#include "gdef.h"
#include "glog.h"
#include "gexception.h"
#include "gstringview.h"
#include <string>
#include <vector>
#include <fstream>
#include <ctime>
#include <array>

namespace G
{
	class LogOutput ;
}

//| \class G::LogOutput
/// Controls and implements low-level logging output, as used by G::Log.
///
/// Applications should instantiate a LogOutput object in main() to
/// enable and configure log output.
///
/// The implementation uses a file descriptor for osoutput() rather than
/// a stream because windows file-sharing options are not accessible
/// when building with mingw streams, and to avoid double buffering.
///
/// \see G::Log
///
class G::LogOutput
{
public:
	G_EXCEPTION( LogFileError , tx("cannot open log file") ) ;
	G_EXCEPTION( EventLogError , tx("cannot access the system event log") ) ;

	enum class SyslogFacility
	{
		User ,
		Daemon ,
		Mail ,
		Cron ,
		Local0 ,
		Local1 ,
		Local2 ,
		Local3 ,
		Local4 ,
		Local5 ,
		Local6 ,
		Local7
	} ;

	struct Config /// A configuration structure for G::LogOutput.
	{
		bool m_output_enabled{false} ;
		bool m_summary_info{false} ;
		bool m_verbose_info{false} ;
		bool m_debug{false} ;
		bool m_with_level{false} ;
		bool m_with_timestamp{false} ;
		bool m_with_context{false} ;
		bool m_strip{false} ; // strip first word
		bool m_quiet_stderr{false} ;
		bool m_use_syslog{false} ;
		bool m_allow_bad_syslog{false} ;
		SyslogFacility m_facility{SyslogFacility::User} ;
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
		Config & set_allow_bad_syslog( bool value = true ) ;
		Config & set_facility( SyslogFacility ) ;
	} ;

	LogOutput( const std::string & exename , const Config & config ,
		const std::string & filename = {} ) ;
			///< Constructor. If there is no LogOutput object, or if
			///< 'config.output_enabled' is false, then there is no
			///< output at all except for assertions to stderr in a
			///< debug build. Otherwise at least warning and error
			///< messages are generated.
			///<
			///< If 'config.summary_info' is true then log-summary
			///< messages are output. If 'config.verbose_info' is true
			///< then log-verbose messages are output. If 'config.with_debug'
			///< is true then debug messages will also be generated (but
			///< only if compiled in).
			///<
			///< If an output filename is given it has a "%d" substitution
			///< applied and it is then opened or created before this
			///< constructor returns. If no filename is given then
			///< logging is sent to the standard error stream; the user
			///< is free to close stderr and reopen it onto /dev/null if
			///< only syslog logging is required.
			///<
			///< More than one LogOutput object may be created, but only
			///< the first one controls output.

	explicit LogOutput( bool output_enabled_and_summary_info ,
		bool verbose_info_and_debug = true ,
		const std::string & filename = {} ) ;
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

	bool at( Log::Severity ) const noexcept ;
		///< Returns true if logging should occur for the given severity level.

	static void context( std::string (*fn)(void*) = nullptr , void * fn_arg = nullptr ) noexcept ;
		///< Sets a functor that is used to provide a context string for
		///< every log line, if configured. The functor should return
		///< the context string with trailing punctuation, typically
		///< colon and space.

	static void * contextarg() noexcept ;
		///< Returns the functor argument as set by the last call to context().

	static std::ostream & start( Log::Severity , const char * file , int line ) ;
		///< Prepares the internal ostream for a new log line and returns a
		///< reference to it. The caller should stream out the rest of the log
		///< line into the ostream and then call output(). Calls to start() and
		///< output() must be strictly in pairs. Returns a pointer to a dummy
		///< ostream if there is no LogOutput instance.

	static void output( std::ostream & ) ;
		///< Emits the current log line (see start()). Does nothing if there
		///< is no LogOutput instance.

	static void assertion( const char * file , int line , bool test , const char * test_string ) ;
		///< Performs an assertion check.

	static void assertion( const char * file , int line , void * test , const char * test_string ) ;
		///< Performs an assertion check. This overload, using a test on a
		///< pointer, is motivated by MSVC warnings.

	static void assertionFailure( const char * file , int line , const char * test_expression ) noexcept ;
		///< Reports an assertion failure.

	GDEF_NORETURN1 static void assertionAbort() GDEF_NORETURN2 ;
		///< Aborts the program when an assertion has failed.

	static void register_( const std::string & exe ) ;
		///< Registers the given executable as a source of logging.
		///< This is called from osinit(), but it might also need to be
		///< done as a program installation step with the necessary
		///< process permissions.

	static void translate( const std::string & info , const std::string & warning ,
		const std::string & error , const std::string & fatal ) ;
			///< Sets the prefix string for the various log levels
			///< (including trailing punctuation).

public:
	LogOutput( const LogOutput & ) = delete ;
	LogOutput( LogOutput && ) = delete ;
	void operator=( const LogOutput & ) = delete ;
	void operator=( LogOutput && ) = delete ;

private:
	void osinit() ;
	void open( std::string , bool ) ;
	std::ostream & start( Log::Severity ) ;
	void output( std::ostream & , int ) ;
	void osoutput( int , Log::Severity , char * , std::size_t ) ;
	void oscleanup() const noexcept ;
	bool updateTime() ;
	void appendTimeTo( std::ostream & ) ;
	static G::string_view levelString( Log::Severity ) noexcept ;
	static const char * basename( const char * ) noexcept ;

private:
	std::string m_exename ;
	Config m_config ;
	std::time_t m_time_s{0} ;
	unsigned int m_time_us{0U} ;
	std::vector<char> m_time_buffer ;
	std::array<char,8U> m_date_buffer {} ;
	HANDLE m_handle{0} ; // windows
	std::string m_path ;
	int m_fd{-1} ;
	unsigned int m_depth{0U} ;
	Log::Severity m_severity{Log::Severity::Debug} ;
	std::size_t m_start_pos{0U} ;
	std::string (*m_context_fn)(void *){nullptr} ;
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
