//
// Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
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
///	G::Log::stream() 
///		<< G::Log::Line(__FILE__,__LINE__)
///		<< a << b << G::Log::end() ;
// or
///	G_LOG( a << b ) ;
//
// See also: LogOutput
//
class G::Log 
{ 
public:
	typedef std::ostream Stream ;
	enum Severity { s_LogVerbose , s_LogSummary , s_Debug , s_Warning , s_Error , s_Assertion } ;

	struct End // A private implementation class for Log. An End object from end() must be streamed out to flush a spooled message.
		{ Severity m_s ; End(Severity s) : m_s(s) {} } ;

	class Line // A class for adding line number information to the Log output.
		{ public: Line( const char *file , int line ) ; } ;

	static End end( Severity severity ) ;
		// Returns an End object which can be used to close off a quantum 
		// of logging. (The End::op<<() function calls G::Log::onEnd().)

	static Stream & stream() ;
		// Returns a stream for streaming messages into.

	static void onEnd( Severity s ) ;
		// A pseudo-private method used by ::operator<<(End).

private:
	friend class G::Log::Line ;
	static void setFile( const char *file ) ;
	static void setLine( int line ) ;
	Log() ;
} ;

namespace G
{
	inline
	std::ostream & operator<<( std::ostream & stream , const G::Log::Line & )
	{
		return stream ;
	}
}

namespace G
{
	inline
	std::ostream & operator<<( std::ostream & stream , const G::Log::End & end )
	{
		G::Log::onEnd( end.m_s ) ;
		return stream ;
	}
}

namespace G
{
	inline
	G::Log::Line::Line( const char * file , int line )
	{
		G::Log::setFile( file ) ;
		G::Log::setLine( line ) ;
	}
}

// Macros: G_LOG, G_LOG_S, G_DEBUG, G_WARNING, G_ERROR
// The debug macro is for debugging during development. The log macro 
// is used for progress logging, typically in long-lived server processes.
// The warning and error macros are used for error warning/error messages. 
// In programs where logging can be disabled completely (see LogOutput) 
// then warning/error messages should also get raised by some another 
// independent means.
//
// (The G_LOG_OUTPUT implementation uses separate statements because
// some compilers dont do Koenig lookup with nested classes. They
// look for ::operator<<() or G::X::operator<<() rather than 
// G::operator<<().)
//
#define G_LOG_OUTPUT( expr , severity ) do { G::Log::Stream & s_ = G::Log::stream() ; G::operator<<( s_ , G::Log::Line(__FILE__,__LINE__) ) ; s_ << expr ; G::operator<<( s_ , G::Log::end(severity) ) ; } while(0)
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
