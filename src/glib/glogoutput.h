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
///
/// \file glogoutput.h
///

#ifndef G_LOG_OUTPUT_H
#define G_LOG_OUTPUT_H

#include "gdef.h"
#include "glog.h"
#include <string>
#include <vector>
#include <set>
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
 	enum SyslogFacility { User , Daemon , Mail , Cron } ; // etc.

	LogOutput( const std::string & prefix , bool output , bool summary_logging ,
		bool verbose_logging , bool with_debug , bool with_level ,
		bool with_timestamp , bool strip_context , bool use_syslog ,
		const std::string & stderr_replacement = std::string() ,
		SyslogFacility syslog_facility = User ) ;
			///< Constructor. If there is no LogOutput object, or if 'output'
			///< is false, then there is no output of any sort. Otherwise at
			///< least warning and error messages are generated.
			///<
			///< If 'summary_logging' is true then log-summary messages
			///< are output. If 'verbose_logging' is true then log-verbose
			///< messages are output. If 'with_debug' is true then debug
			///< messages will also be generated (but only if compiled in).
			///<
			///< More than one LogOutput object may be created, but only
			///< the first one controls output.

	explicit LogOutput( bool output_with_logging , bool verbose_and_debug = true ,
		const std::string & stderr_replacement = std::string() ) ;
			///< Constructor for test programs. Only generates output if the
			///< first parameter is true. Never uses syslog.

	virtual ~LogOutput() ;
		///< Destructor.

	virtual void rawOutput( std::ostream & , G::Log::Severity , const std::string & ) ;
		///< Overridable. Used to do the final message
		///< output (with OutputDebugString() or stderr).

	static LogOutput * instance() ;
		///< Returns a pointer to the controlling LogOutput object. Returns
		///< nullptr if none.

	bool enable( bool enabled = true ) ;
		///< Enables or disables output. Returns the previous setting.

	void quiet( bool quiet_stderr = true ) ;
		///< Enables or disables quiet-stderr mode, intended for daemons
		///< that log to syslog and only want startup errors and warnings
		///< on stderr.

	void verbose( bool verbose_log = true ) ;
		///< Enables or disables verbose logging.

	bool at( G::Log::Severity ) const ;
		///< Returns true if logging should occur for the given severity level.

	bool at( G::Log::Severity , const std::string & group ) const ;
		///< Returns true if logging should occur for the given severity level
		///< and logging group.

	static void groups( const std::string & ) ;
		///< Sets the list of active logging groups. The string is a comma-separated
		///< list of "log-whatever" names, with "log-all" having the obvious meaning.
		///< Names that do not start with "log-" are ignored. The default set of
		///< groups is taken from an environment variable on construction. Does
		///< nothing if there is no instance().

	bool syslog() const ;
		///< Returns true if syslog output is enabled.

	static void output( G::Log::Severity , const char * file , int line , const std::string & ) ;
		///< Generates output if there is an existing LogOutput object
		///< which is enabled. Uses rawOutput().

	static void assertion( const char * file , int line , bool test , const char * ) ;
		///< Makes an assertion check (regardless of any LogOutput
		///< object). Calls output() if the 'file' parameter is not null.

	virtual void onAssert() ;
		///< Called during an assertion failure. This allows Windows
		///< applications to stop timers etc. (Timers can cause reentrancy
		///< problems and infinitely recursive dialog box creation.)

	static void register_( const std::string & exe ) ;
		///< Registers the given executable as a source of logging.
		///< This is called from init(), but it might also need to be
		///< done as an installation task with the necessary
		///< permissions.

private:
	typedef size_t size_type ;
	LogOutput( const LogOutput & ) ; // not implemented
	void operator=( const LogOutput & ) ; // not implemented
	void init() ;
	void cleanup() ;
	void appendTimestampStringTo( std::string & ) ;
	static std::string dateString() ;
	void doOutput( G::Log::Severity , const std::string & ) ;
	void doOutput( G::Log::Severity s , const char * , int , const std::string & ) ;
	void doAssertion( const char * , int , const std::string & ) ;
	static const char * levelString( Log::Severity s ) ;
	static std::string itoa( int ) ;
	static std::string fileAndLine( const char * , int ) ;
	static void halt() ;
	static LogOutput * & pthis() ;
	static std::ostream & err( const std::string & ) ;
	static void getLocalTime( time_t , struct std::tm * ) ; // dont make logging dependent on G::DateTime

private:
	std::string m_prefix ;
	bool m_enabled ;
	bool m_summary_log ;
	bool m_verbose_log ;
	bool m_quiet ;
	bool m_debug ;
	bool m_level ;
	bool m_strip ;
	bool m_syslog ;
	std::set<std::string> m_groups ;
	std::ostream & m_std_err ;
	SyslogFacility m_facility ;
	time_t m_time ;
	std::vector<char> m_time_buffer ;
	bool m_timestamp ;
	HANDLE m_handle ; // windows
	bool m_handle_set ;
	std::string m_buffer ;
} ;

#endif
