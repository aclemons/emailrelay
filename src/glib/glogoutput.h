//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
///
/// \file glogoutput.h
///
	
#ifndef G_LOG_OUTPUT_H
#define G_LOG_OUTPUT_H

#include "gdef.h"
#include "glog.h"

/// \namespace G
namespace G
{
	class LogOutput ;
}

/// \class G::LogOutput
/// Controls and implements low-level logging output, as used by the Log interface.
/// Applications should normally instantiate a LogOutput object in main() to enable
/// log output.
/// \see G::Log
///
class G::LogOutput 
{
public:
 	enum SyslogFacility { User , Daemon , Mail , Cron } ; // etc.

	LogOutput( const std::string & prefix , bool output , bool with_logging , 
		bool with_verbose_logging , bool with_debug , bool with_level ,
		bool with_timestamp , bool strip_context ,
		bool use_syslog , SyslogFacility syslog_facility = User ) ;
			///< Constructor. If there is no LogOutput object,
			///< or if 'output' is false, then there is no 
			///< output of any sort. Otherwise at least
			///< warning and error messages are generated.
			///<
			///< If 'with-logging' is true then log[summary] messages 
			///< are output. If 'with-verbose-logging' is true then 
			///< log[verbose] messages are output. If 'with_debug' is 
			///< true then debug messages will also be generated
			///< (but only if compiled in).
			///<
			///< More than one LogOutput object may be created, but 
			///< only the first one controls output.

	explicit LogOutput( bool output_with_logging , bool verbose_and_debug = true ) ;
		///< Constructor.

	virtual ~LogOutput() ;
		///< Destructor.

	virtual void rawOutput( G::Log::Severity s , const char *string ) ;
		///< Overridable. Used to do the final message 
		///< output (with OutputDebugString() or stderr).
		
	static LogOutput * instance() ;
		///< Returns a pointer to the controlling
		///< LogOutput object. Returns NULL if none.
		
	bool enable( bool enabled = true ) ;
		///< Enables or disables output.
		///< Returns the previous setting.

	static void output( G::Log::Severity s , const char *file , unsigned line , const char *text ) ;
		///< Generates output if there is an existing
		///< LogOutput object which is enabled. Uses rawOutput().

	static void assertion( const char *file , unsigned line , bool test , const char *test_string ) ;	
		///< Makes an assertion check (regardless of any LogOutput
		///< object). Calls output() if the 'file' parameter is 
		///< not null.

	virtual void onAssert() ;
		///< Called during an assertion failure. This allows
		///< Windows applications to stop timers etc. (Timers
		///< can cause reentrancy problems and infinitely 
		///< recursive dialog box creation.)

private:
	LogOutput( const LogOutput & ) ;
	void operator=( const LogOutput & ) ;
	static const char * itoa( char * , size_t , unsigned int ) ;
	static void addFileAndLine( char * , size_t , const char * , int ) ;
	static void add( char * , size_t , const char * ) ;
	static void add( char * , size_t , const std::string & ) ;
	const char * timestampString() ;
	static void halt() ;
	void doOutput( G::Log::Severity , const char * ) ;
	void doOutput( G::Log::Severity s , const char * , unsigned , const char * ) ;
	void doAssertion( const char * , unsigned , const char * ) ;
	const char * levelString( Log::Severity s ) ;
	void init() ;
	void cleanup() ;

private:
	static LogOutput * m_this ;
	std::string m_prefix ;
	bool m_enabled ;
	bool m_summary_log ;
	bool m_verbose_log ;
	bool m_debug ;
	bool m_level ;
	bool m_strip ;
	bool m_syslog ;
	SyslogFacility m_facility ;
	time_t m_time ;
	char m_time_buffer[40U] ;
	bool m_timestamp ;
	HANDLE m_handle ; // windows
	bool m_handle_set ;
} ;

#endif
