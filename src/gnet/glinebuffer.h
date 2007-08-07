//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file glinebuffer.h
///

#ifndef G_LINE_BUFFER_H
#define G_LINE_BUFFER_H

#include "gdef.h"
#include "gnet.h"
#include "gexception.h"
#include <string>

/// \namespace GNet
namespace GNet
{
	class LineBuffer ;
}

/// \class GNet::LineBuffer
/// A class which does line buffering. Raw
/// data is added, and newline-delimited lines are
/// extracted.
/// Usage:
/// \code
/// {
///   GNet::LineBuffer buffer ;
///   buffer.add("abc") ;
///   buffer.add("def\nABC\nDE") ;
///   buffer.add("F\n") ;
///   while( buffer.more() )
///     cout << buffer.line() << endl ;
/// }
/// \endcode
///
class GNet::LineBuffer 
{
public:
	G_EXCEPTION( Overflow , "line buffer overflow: maximum input line length exceeded" ) ;

	explicit LineBuffer( const std::string & eol = std::string("\n") , bool do_throw_on_overflow = false ) ;
		///< Constructor.

	void add( const std::string & segment ) ;
		///< Adds a data segment.

	void add( const char * p , std::string::size_type n ) ;
		///< Adds a data segment.

	bool more() const ;
		///< Returns true if there are complete 
		///< line(s) to be extracted.

	const std::string & current() const ;
		///< Returns the current line, without extracting 
		///< it. The line terminator is not included.
		///<
		///< Precondition: more()

	void discard() ;
		///< Discards the current line.
		///<
		///< Precondition: more()

	std::string line() ;
		///< Extracts a line and returns it as a string. 
		///< The line terminator is not included.

private:
	LineBuffer( const LineBuffer & ) ;
	void operator=( const LineBuffer & ) ;
	void load() ;
	void check( std::string::size_type ) ;

private:
	static unsigned long m_limit ;
	std::string m_eol ;
	std::string m_current ;
	std::string m_store ;
	bool m_more ;
	bool m_throw ;
} ;

#endif

