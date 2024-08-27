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
/// \file glogoutput.h
///

#ifndef G_LOG_OUTPUT_H
#define G_LOG_OUTPUT_H

#include "gdef.h"
#include "glogstream.h"
#include "gprocess.h"
#include "gpath.h"
#include "glimits.h"
#include "gomembuf.h"
#include "gexception.h"
#include "gstringview.h"
#include <string>
#include <vector>
#include <fstream>
#include <ctime>
#include <array>
#include <utility>

#ifndef G_LOG_THREAD_LOCAL
#define G_LOG_THREAD_LOCAL
#endif

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
/// when building with mingw streams, and to avoid double buffering
/// and mixed output from multiple threads.
///
/// \see G::Log
///
class G::LogOutput
{
public:
	G_EXCEPTION( LogFileError , tx("cannot open log file") )
	G_EXCEPTION( EventLogError , tx("cannot access the system event log") )

	enum class Severity
	{
		Debug ,
		InfoMoreVerbose ,
		InfoVerbose ,
		InfoSummary ,
		Warning ,
		Error ,
		Assertion
	} ;

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
		bool m_output_enabled {false} ;
		bool m_summary_info {false} ;
		bool m_verbose_info {false} ;
		bool m_more_verbose_info {false} ;
		bool m_debug {false} ;
		bool m_with_level {false} ;
		bool m_with_timestamp {false} ;
		bool m_with_context {false} ;
		bool m_strip {false} ; // strip first word
		bool m_quiet_stderr {false} ;
		bool m_use_syslog {false} ;
		bool m_allow_bad_syslog {false} ;
		bool m_stdout {false} ;
		SyslogFacility m_facility {SyslogFacility::User} ;
		Process::Umask::Mode m_umask {Process::Umask::Mode::NoChange} ;
		Config() noexcept ;
		Config( bool enabled , bool verbose ) noexcept ;
		Config & set_output_enabled( bool value = true ) noexcept ;
		Config & set_summary_info( bool value = true ) noexcept ;
		Config & set_verbose_info( bool value = true ) noexcept ;
		Config & set_more_verbose_info( bool value = true ) noexcept ;
		Config & set_debug( bool value = true ) noexcept ;
		Config & set_with_level( bool value = true ) noexcept ;
		Config & set_with_timestamp( bool value = true ) noexcept ;
		Config & set_with_context( bool value = true ) noexcept ;
		Config & set_strip( bool value = true ) noexcept ;
		Config & set_quiet_stderr( bool value = true ) noexcept ;
		Config & set_use_syslog( bool value = true ) noexcept ;
		Config & set_allow_bad_syslog( bool value = true ) noexcept ;
		Config & set_facility( SyslogFacility ) noexcept ;
		Config & set_umask( Process::Umask::Mode ) noexcept ;
		Config & set_stdout( bool value = true ) noexcept ;
	} ;

	struct Instance /// A set of convenience functions for calling LogOutput methods on LogOutput::instance().
	{
		static bool at( LogOutput::Severity ) noexcept ;
		static bool atVerbose() noexcept ;
		static bool atDebug() noexcept ;
		static void context( std::string_view (*fn)(void*) = nullptr , void * fn_arg = nullptr ) noexcept ;
		static void * contextarg() noexcept ;
		static LogStream start( LogOutput::Severity , const char * file , int line ) noexcept ;
		static void output( LogStream & ) noexcept ;
		static int fd() noexcept ;
		static LogOutput::Config config() noexcept ;
		Instance() = delete ;
	} ;

	LogOutput( const std::string & exename , const Config & config ,
		const Path & logfile = {} ) ;
			///< Constructor. If there is no LogOutput object, or if
			///< 'config.output_enabled' is false, then there is no
			///< output at all except for assertions to stderr.
			///<
			///< The 'exename' value is emitted as an additional prefix
			///< on each log line if it is non-empty. See also
			///< G::Arg::prefix().
			///<
			///< The log-file path can contain substitution variables
			///< "%d" or "%h" for the current day and hour respectively.
			///< If no path is given then logging is sent to the standard
			///< error stream by default; stderr should be re-opened
			///< onto /dev/null if only syslog logging is required.
			///<
			///< More than one LogOutput object may be created but
			///< only the first one controls output (see instance()).

	LogOutput( const std::string & exename , const Config & config , int fd ) ;
		///< Constructor overload taking a file descriptor.
		///< Typically used for non-main threads when instance()
		///< is thread-local.

	explicit LogOutput( bool enabled , bool verbose = true , const G::Path & logfile = {} ) ;
		///< Constructor overload for test programs.

	~LogOutput() ;
		///< Destructor.

	static LogOutput * instance() noexcept ;
		///< Returns a pointer to the controlling LogOutput object.
		///< Returns nullptr if none.
		///<
		///< Non-main threads can do logging safely if the (private)
		///< instance pointer is declared 'thread_local' at build-time.
		///< Thread functions can then instantiate their own LogOutput
		///< object on the stack, typically with the Config object
		///< and file descriptor passed in from the main thread.

	Config config() const noexcept ;
		///< Returns the current configuration.

	int fd() const noexcept ;
		///< Returns the output file descriptor.

	void configure( const Config & ) ;
		///< Updates the current configuration.

	bool at( Severity ) const noexcept ;
		///< Returns true if logging should occur for the given severity level.

	void context( std::string_view (*fn)(void*) = nullptr , void * fn_arg = nullptr ) noexcept ;
		///< Sets a functor that is used to provide a context string for
		///< every log line, if configured. The functor should return
		///< the context string with trailing punctuation, typically
		///< colon and space.

	void * contextarg() noexcept ;
		///< Returns the functor argument as set by the last call to context().

	LogStream start( Severity , const char * file , int line ) noexcept ;
		///< Returns an ostream for a new log line. The caller should stream out
		///< the rest of the log line into the ostream and then call output().
		///< Calls to start() and output() should be in pairs.

	void output( LogStream & ) noexcept ;
		///< Emits the current log line (see start()). Does nothing if there
		///< is no LogOutput instance.

	static void assertion( LogOutput * , const char * file , int line , bool test , const char * test_string ) ;
		///< Performs an assertion check.

	static void assertion( LogOutput * , const char * file , int line , void * test , const char * test_string ) ;
		///< Performs an assertion check. This overload, using a test on a
		///< pointer, is motivated by MSVC warnings.

	static void assertionFailure( LogOutput * , const char * file , int line , const char * test_string ) noexcept ;
		///< Reports an assertion failure.

	GDEF_NORETURN_LHS static void assertionAbort() GDEF_NORETURN_RHS ;
		///< Aborts the program when an assertion has failed.

	static void register_( const Path & exe ) ;
		///< Registers the given executable as a source of logging.
		///< This is called from osinit(), but it might also need to be
		///< done as a program installation step with the necessary
		///< process permissions.

public:
	LogOutput( const LogOutput & ) = delete ;
	LogOutput( LogOutput && ) = delete ;
	LogOutput & operator=( const LogOutput & ) = delete ;
	LogOutput & operator=( LogOutput && ) = delete ;

private:
	struct Private {} ;
	LogOutput( Private , const std::string & , const Config & ) ;
	void init() ;
	void osinit() ;
	void open( const Path & , bool ) ;
	LogStream start( Severity ) ;
	void output( LogStream & , int ) ;
	void osoutput( int , Severity , char * , std::size_t ) ;
	void oscleanup() const noexcept ;
	bool updateTime() ;
	bool updatePath( const Path & , Path & ) const ;
	Path makePath( const Path & ) const ;
	void appendTimeTo( LogStream & ) ;
	static std::string_view levelString( Severity ) noexcept ;
	static const char * basename( const char * ) noexcept ;

private:
	struct Stream : public std::ostream
	{
		explicit Stream( omembuf * buf ) : std::ostream(buf) {}
		void reset() { clear() ; seekp(0) ; }
	} ;

private:
	friend struct Instance ;
	static G_LOG_THREAD_LOCAL LogOutput * m_instance ;
	std::string m_exename ;
	Config m_config ;
	std::time_t m_time_s {0} ;
	unsigned int m_time_us {0U} ;
	std::array<char,17U> m_time_buffer {} ;
	std::array<char,17U> m_time_change_buffer {} ;
	static constexpr std::size_t m_rhs_margin = 7U ;
	static constexpr std::size_t m_buffer_base_size = Limits<>::log + 40U ;
	static constexpr std::size_t m_buffer_size = m_buffer_base_size + m_rhs_margin ;
	std::vector<char> m_buffer ;
	G::omembuf m_streambuf ; // omembuf over m_buffer
	Stream m_stream ; // std::ostream over m_streambuf
	HANDLE m_handle {0} ; // windows only
	Path m_path ; // with %d etc
	Path m_real_path ;
	int m_fd {-1} ;
	unsigned int m_depth {0U} ;
	Severity m_severity {Severity::Debug} ;
	std::size_t m_start_pos {0U} ;
	std::string_view (*m_context_fn)(void *) {nullptr} ;
	void * m_context_fn_arg {nullptr} ;
} ;

inline void G::LogOutput::assertion( LogOutput * instance , const char * file , int line , bool test , const char * test_string )
{
	if( !test )
	{
		assertionFailure( instance , file , line , test_string ) ;
		assertionAbort() ;
	}
}

inline void G::LogOutput::assertion( LogOutput * instance , const char * file , int line , void * test , const char * test_string )
{
	if( !test )
	{
		assertionFailure( instance , file , line , test_string ) ;
		assertionAbort() ;
	}
}

inline G::LogOutput::Config & G::LogOutput::Config::set_output_enabled( bool value ) noexcept { m_output_enabled = value ; return *this ; }
inline G::LogOutput::Config & G::LogOutput::Config::set_summary_info( bool value ) noexcept { m_summary_info = value ; return *this ; }
inline G::LogOutput::Config & G::LogOutput::Config::set_verbose_info( bool value ) noexcept { m_verbose_info = value ; return *this ; }
inline G::LogOutput::Config & G::LogOutput::Config::set_more_verbose_info( bool value ) noexcept { m_more_verbose_info = value ; return *this ; }
inline G::LogOutput::Config & G::LogOutput::Config::set_debug( bool value ) noexcept { m_debug = value ; return *this ; }
inline G::LogOutput::Config & G::LogOutput::Config::set_with_level( bool value ) noexcept { m_with_level = value ; return *this ; }
inline G::LogOutput::Config & G::LogOutput::Config::set_with_timestamp( bool value ) noexcept { m_with_timestamp = value ; return *this ; }
inline G::LogOutput::Config & G::LogOutput::Config::set_with_context( bool value ) noexcept { m_with_context = value ; return *this ; }
inline G::LogOutput::Config & G::LogOutput::Config::set_strip( bool value ) noexcept { m_strip = value ; return *this ; }
inline G::LogOutput::Config & G::LogOutput::Config::set_quiet_stderr( bool value ) noexcept { m_quiet_stderr = value ; return *this ; }
inline G::LogOutput::Config & G::LogOutput::Config::set_use_syslog( bool value ) noexcept { m_use_syslog = value ; return *this ; }
inline G::LogOutput::Config & G::LogOutput::Config::set_allow_bad_syslog( bool value ) noexcept { m_allow_bad_syslog = value ; return *this ; }
inline G::LogOutput::Config & G::LogOutput::Config::set_facility( SyslogFacility facility ) noexcept { m_facility = facility ; return *this ; }
inline G::LogOutput::Config & G::LogOutput::Config::set_umask( Process::Umask::Mode umask ) noexcept { m_umask = umask ; return *this ; }
inline G::LogOutput::Config & G::LogOutput::Config::set_stdout( bool value ) noexcept { m_stdout = value ; return *this ; }

inline bool G::LogOutput::Instance::at( LogOutput::Severity s ) noexcept { return LogOutput::m_instance && LogOutput::m_instance->at( s ) ; }
inline bool G::LogOutput::Instance::atVerbose() noexcept { return LogOutput::m_instance && LogOutput::m_instance->at( LogOutput::Severity::InfoVerbose ) ; }
inline bool G::LogOutput::Instance::atDebug() noexcept { return LogOutput::m_instance && LogOutput::m_instance->at( LogOutput::Severity::Debug ) ; }
inline void G::LogOutput::Instance::context( std::string_view (*fn)(void*) , void * fn_arg ) noexcept { if( LogOutput::m_instance ) LogOutput::m_instance->context( fn , fn_arg ) ; }
inline void * G::LogOutput::Instance::contextarg() noexcept { if( LogOutput::m_instance ) return LogOutput::m_instance->contextarg() ; else return nullptr ; }
inline G::LogStream G::LogOutput::Instance::start( LogOutput::Severity s , const char * file , int line ) noexcept { if( LogOutput::m_instance ) return LogOutput::m_instance->start( s , file , line ) ; else return LogStream( nullptr ) ; }
inline void G::LogOutput::Instance::output( LogStream & stream ) noexcept { if( LogOutput::m_instance ) LogOutput::m_instance->output( stream ) ; }
inline int G::LogOutput::Instance::fd() noexcept { return LogOutput::m_instance ? LogOutput::m_instance->fd() : -1 ; }
inline G::LogOutput::Config G::LogOutput::Instance::config() noexcept { return LogOutput::m_instance ? LogOutput::m_instance->config() : LogOutput::Config() ; }

#endif
