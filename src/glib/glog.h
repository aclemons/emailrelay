//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
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
//
// glog.h
//

#ifndef G_LOG_H
#define G_LOG_H

#include "gdef.h"
#include <sstream>
#include <string>

namespace G
{
	class Log ;
}

// Class: G::Log
// Description: A static class for doing iostream-based logging.
// The G_LOG/G_DEBUG/G_WARNING/G_ERROR macros are provided as a 
// convenient way of using this interface.
//
// Usage:
///	G::Log(G::Log::s_LogSummary,__FILE__,__LINE__) << a << b ;
// or
///	G_LOG( a << b ) ;
//
// See also: G::LogOutput
//
class G::Log 
{ 
public:
	enum Severity { s_LogVerbose , s_LogSummary , s_Debug , s_Warning , s_Error , s_Assertion } ;

	class Line // A class for adding line number information to the Log output.
		{ public: const char * m_file ; int m_line ; Line( const char *file , int line ) : m_file(file) , m_line(line) {} } ;

	Log( Severity , const char * file , int line ) ;
		// Constructor.

	~Log() ;
		// Destructor. Writes the accumulated string to the log output.

	std::ostream & operator<<( const char * s ) ;
		// Streams 's' and then returns a stream for streaming more stuff into.

	std::ostream & operator<<( const std::string & s ) ;
		// Streams 's' and then returns a stream for streaming more stuff into.

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

// Macros: G_LOG, G_LOG_S, G_DEBUG, G_WARNING, G_ERROR
// The debug macro is for debugging during development. The log macro 
// is used for progress logging, typically in long-lived server processes.
// The warning and error macros are used for error warning/error messages. 
// In programs where logging can be disabled completely (see LogOutput) 
// then warning/error messages should also get raised by some another 
// independent means.
//
#define G_LOG_OUTPUT( expr , severity ) do { G::Log(severity,__FILE__,__LINE__) << expr ; } while(0)
#if defined(_DEBUG) && ! defined(G_NO_DEBUG) 
#define G_DEBUG( expr ) G_LOG_OUTPUT( expr , G::Log::s_Debug )
#else
#define G_DEBUG( expr )
#endif
#if ! defined(G_NO_LOG)
#define G_LOG( expr ) G_LOG_OUTPUT( expr , G::Log::s_LogVerbose )
#define G_LOG_S( expr ) G_LOG_OUTPUT( expr , G::Log::s_LogSummary )
#else
#define G_LOG( expr )
#define G_LOG_S( expr )
#endif
#define G_WARNING( expr ) G_LOG_OUTPUT( expr , G::Log::s_Warning )
#define G_ERROR( expr ) G_LOG_OUTPUT( expr , G::Log::s_Error )

#endif
