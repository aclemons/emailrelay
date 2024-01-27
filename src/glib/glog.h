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
/// \file glog.h
///

#ifndef G_LOG_H
#define G_LOG_H

#include "gdef.h"
#include "glogstream.h"
#include "gformat.h"
#include <sstream>
#include <string>

namespace G
{
	class Log ;
}

//| \class G::Log
/// A class for doing iostream-based logging. The G_LOG/G_DEBUG/G_WARNING/G_ERROR
/// macros are provided as a convenient way of using this interface.
///
/// Usage:
/// \code
///	G::Log(G::Log::Severity::InfoSummary,__FILE__,__LINE__) << a << b ;
/// \endcode
/// or
/// \code
///	G_LOG( a << b ) ;
/// \endcode
/// or
/// \code
/// G_LOG( G::format("%1% %2%") % a % b ) ;
/// \endcode
///
/// \see G::LogOutput
///
class G::Log
{
public:
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

	Log( Severity , const char * file , int line ) noexcept ;
		///< Constructor.

	~Log() ;
		///< Destructor. Writes the accumulated string to the log output.

	LogStream & operator<<( const char * s ) noexcept ;
		///< Streams 's' and then returns a stream for streaming more stuff into.

	LogStream & operator<<( const std::string & s ) noexcept ;
		///< Streams 's' and then returns a stream for streaming more stuff into.

	LogStream & operator<<( const format & f ) ;
		///< Streams 'f' and then returns a stream for streaming more stuff into.

	static bool at( Severity ) noexcept ;
		///< Returns true if G::LogOutput::output() would log at the given level.
		///< This can be used as an optimisation to short-ciruit the stream-out
		///< expression evaluation.

	static bool atDebug() noexcept ;
		///< Returns at(Severity::Debug).

	static bool atVerbose() noexcept ;
		///< Returns at(Severity::InfoVerbose).

	static bool atMoreVerbose() noexcept ;
		///< Returns at(Severity::InfoMoreVerbose).

public:
	Log( const Log & ) = delete ;
	Log( Log && ) = delete ;
	Log & operator=( const Log & ) = delete ;
	Log & operator=( Log && ) = delete ;

private:
	void flush() ;

private:
	Severity m_severity ;
	const char * m_file ;
	int m_line ;
	LogStream & m_logstream ;
} ;

inline bool G::Log::atDebug() noexcept
{
	return at( Log::Severity::Debug ) ;
}

inline bool G::Log::atVerbose() noexcept
{
	return at( Log::Severity::InfoVerbose ) ;
}

inline bool G::Log::atMoreVerbose() noexcept
{
	return at( Log::Severity::InfoMoreVerbose ) ;
}

// Macros: G_LOG_S, G_LOG, G_LOG_MORE, G_DEBUG, G_WARNING, G_ERROR
// The G_DEBUG macro is for debugging during development, the G_LOG macro
// generates informational logging in verbose mode only, the 'summary'
// G_LOG_S macro generates informational logging even when not verbose,
// and the G_WARNING and G_ERROR macros are used for error warning/error
// messages although in programs where logging can be disabled completely (see
// G::LogOutput) error conditions should be made visible by some other means
// (such as stderr).

#define G_LOG_IMP( expr , severity ) do { if(G::Log::at(severity)) G::Log((severity),__FILE__,__LINE__) << expr ; } while(0) /* NOLINT bugprone-macro-parentheses */
#define G_LOG_IMP_IF( cond , expr , severity ) do { if(G::Log::at(severity)&&(cond)) G::Log((severity),__FILE__,__LINE__) << expr ; } while(0) /* NOLINT bugprone-macro-parentheses */
#define G_LOG_IMP_ONCE( expr , severity ) do { static bool done__ = false ; if(!done__&&G::Log::at(severity)) { G::Log((severity),__FILE__,__LINE__) << expr ;  done__ = true ; } } while(0) /* NOLINT bugprone-macro-parentheses */

#if defined(G_WITH_DEBUG) || ( defined(_DEBUG) && ! defined(G_NO_DEBUG) )
#define G_DEBUG( expr ) G_LOG_IMP( expr , G::Log::Severity::Debug )
#define G_DEBUG_IF( cond , expr ) G_LOG_IMP_IF( cond , expr , G::Log::Severity::Debug )
#define G_DEBUG_ONCE( expr ) G_LOG_IMP_ONCE( expr , G::Log::Severity::Debug )
#else
#define G_DEBUG( expr )
#define G_DEBUG_IF( cond , expr )
#define G_DEBUG_ONCE( group , expr )
#endif

#if ! defined(G_NO_LOG)
#define G_LOG( expr ) G_LOG_IMP( expr , G::Log::Severity::InfoVerbose )
#define G_LOG_IF( cond , expr ) G_LOG_IMP_IF( cond , expr , G::Log::Severity::InfoVerbose )
#define G_LOG_ONCE( expr ) G_LOG_IMP_ONCE( expr , G::Log::Severity::InfoVerbose )
#else
#define G_LOG( expr )
#define G_LOG_IF( cond , expr )
#define G_LOG_ONCE( expr )
#endif

#if ! defined(G_NO_LOG_MORE)
#define G_LOG_MORE( expr ) G_LOG_IMP( expr , G::Log::Severity::InfoMoreVerbose )
#define G_LOG_MORE_IF( cond , expr ) G_LOG_IMP_IF( cond , expr , G::Log::Severity::InfoMoreVerbose )
#define G_LOG_MORE_ONCE( expr ) G_LOG_IMP_ONCE( expr , G::Log::Severity::InfoMoreVerbose )
#else
#define G_LOG_MORE( expr )
#define G_LOG_MORE_IF( cond , expr )
#define G_LOG_MORE_ONCE( expr )
#endif

#if ! defined(G_NO_LOG_S)
#define G_LOG_S( expr ) G_LOG_IMP( expr , G::Log::Severity::InfoSummary )
#define G_LOG_S_IF( cond , expr ) G_LOG_IMP_IF( cond , expr , G::Log::Severity::InfoSummary )
#define G_LOG_S_ONCE( expr ) G_LOG_IMP_ONCE( expr , G::Log::Severity::InfoSummary )
#else
#define G_LOG_S( expr )
#define G_LOG_S_IF( cond , expr )
#define G_LOG_S_ONCE( expr )
#endif

#if ! defined(G_NO_WARNING)
#define G_WARNING( expr ) G_LOG_IMP( expr , G::Log::Severity::Warning )
#define G_WARNING_IF( cond , expr ) G_LOG_IMP_IF( cond , expr , G::Log::Severity::Warning )
#define G_WARNING_ONCE( expr ) G_LOG_IMP_ONCE( expr , G::Log::Severity::Warning )
#else
#define G_WARNING( expr )
#define G_WARNING_IF( cond , expr )
#define G_WARNING_ONCE( expr )
#endif

#if ! defined(G_NO_ERROR)
#define G_ERROR( expr ) G_LOG_IMP( expr , G::Log::Severity::Error )
#else
#define G_ERROR( expr )
#endif

#endif
