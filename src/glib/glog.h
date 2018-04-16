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
/// \file glog.h
///

#ifndef G_LOG_H
#define G_LOG_H

#include "gdef.h"
#include <sstream>
#include <string>

namespace G
{
	class Log ;
}

/// \class G::Log
/// A class for doing iostream-based logging. The G_LOG/G_DEBUG/G_WARNING/G_ERROR
/// macros are provided as a convenient way of using this interface.
///
/// Usage:
/// \code
///	G::Log(G::Log::s_LogSummary,__FILE__,__LINE__) << a << b ;
/// \endcode
/// or
/// \code
///	G_LOG( a << b ) ;
/// \endcode
///
/// \see G::LogOutput
///
class G::Log
{
public:
	enum Severity { s_LogVerbose , s_LogSummary , s_Debug , s_Warning , s_Error , s_Assertion } ;

	class Line /// A class for adding line number information to the Log output.
		{ public: const char * m_file ; int m_line ; Line( const char *file , int line ) : m_file(file) , m_line(line) {} } ;

	Log( Severity , const char * file , int line ) ;
		///< Constructor.

	~Log() ;
		///< Destructor. Writes the accumulated string to the log output.

	std::ostream & operator<<( const char * s ) ;
		///< Streams 's' and then returns a stream for streaming more stuff into.

	std::ostream & operator<<( const std::string & s ) ;
		///< Streams 's' and then returns a stream for streaming more stuff into.

	static bool at( Severity ) ;
		///< Returns true if G::LogOutput::output() would log at the given level.
		///< This can be used as an optimisation to short-ciruit the stream-out
		///< expression evaluation.

	static bool at( Severity , const char * group ) ;
		///< An overload that adds a logging group name to the test.

private:
	void flush() ;
	bool active() ;

private:
	friend class G::Log::Line ;
	Severity m_severity ;
	std::ostringstream m_ss ;
	const char * m_file ;
	int m_line ;
} ;

/// The debug macro is for debugging during development. The log macro
/// is used for progress logging, typically in long-lived server processes.
/// The warning and error macros are used for error warning/error messages.
/// In programs where logging can be disabled completely (see LogOutput)
/// then warning/error messages should also get raised by some another
/// independent means.
///
#define G_LOG_IMP( expr , severity ) do { if(G::Log::at(severity)) G::Log(severity,__FILE__,__LINE__) << expr ; } while(0)
#define G_LOG_IMP_IF( cond , expr , severity ) do { if(G::Log::at(severity)&&(cond)) G::Log(severity,__FILE__,__LINE__) << expr ; } while(0)
#define G_LOG_IMP_GROUP( grp , expr , severity ) do { if(G::Log::at(severity,grp)) G::Log(severity,__FILE__,__LINE__) << expr ; } while(0)
#define G_LOG_IMP_ONCE( expr , severity ) do { static bool done__ = false ; if(!done__) G::Log(severity,__FILE__,__LINE__) << expr ; done__ = true ; } while(0)

#if defined(G_WITH_DEBUG) || ( defined(_DEBUG) && ! defined(G_NO_DEBUG) )
#define G_DEBUG( expr ) G_LOG_IMP( expr , G::Log::s_Debug )
#define G_DEBUG_IF( cond , expr ) G_LOG_IMP_IF( cond , expr , G::Log::s_Debug )
#define G_DEBUG_GROUP( group , expr ) G_LOG_IMP_GROUP( group , expr , G::Log::s_Debug )
#define G_DEBUG_ONCE( expr ) G_LOG_IMP_ONCE( expr , G::Log::s_Debug )
#else
#define G_DEBUG( expr )
#define G_DEBUG_IF( cond , expr )
#define G_DEBUG_GROUP( group , expr )
#define G_DEBUG_ONCE( group , expr )
#endif

#if ! defined(G_NO_LOG)
#define G_LOG( expr ) G_LOG_IMP( expr , G::Log::s_LogVerbose )
#define G_LOG_IF( cond , expr ) G_LOG_IMP_IF( cond , expr , G::Log::s_LogVerbose )
#define G_LOG_GROUP( grp , expr ) G_LOG_IMP_GROUP( grp , expr , G::Log::s_LogVerbose )
#define G_LOG_ONCE( expr ) G_LOG_IMP_ONCE( cond , expr , G::Log::s_LogVerbose )
#else
#define G_LOG( expr )
#define G_LOG_IF( cond , expr )
#define G_LOG_GROUP( grp , expr )
#define G_LOG_ONCE( expr )
#endif

#if ! defined(G_NO_LOG_S)
#define G_LOG_S( expr ) G_LOG_IMP( expr , G::Log::s_LogSummary )
#else
#define G_LOG_S( expr )
#endif

#if ! defined(G_NO_WARNING)
#define G_WARNING( expr ) G_LOG_IMP( expr , G::Log::s_Warning )
#define G_WARNING_ONCE( expr ) G_LOG_IMP_ONCE( expr , G::Log::s_Warning )
#else
#define G_WARNING( expr )
#define G_WARNING_ONCE( expr )
#endif

#if ! defined(G_NO_ERROR)
#define G_ERROR( expr ) G_LOG_IMP( expr , G::Log::s_Error )
#else
#define G_ERROR( expr )
#endif

#endif
