//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// glinebuffer.h
//

#ifndef G_LINE_BUFFER_H
#define G_LINE_BUFFER_H

#include "gdef.h"
#include "gnet.h"
#include "gexception.h"
#include <string>

namespace GNet
{
	class LineBuffer ;
}

// Class: GNet::LineBuffer
// Description: A class which does line buffering. Raw
// data is added, and newline-delimited lines are
// extracted.
// Usage:
/// {
///   GNet::LineBuffer buffer ;
///   buffer.add("abc") ;
///   buffer.add("def\nABC\nDE") ;
///   buffer.add("F\n") ;
///   while( buffer.more() )
///     cout << buffer.line() << endl ;
/// }
//
class GNet::LineBuffer 
{
public:
	G_EXCEPTION( Overflow , "line buffer overflow: maximum input line length exceeded" ) ;

	explicit LineBuffer( const std::string & eol = std::string("\n") , bool do_throw_on_overflow = false ) ;
		// Constructor.

	void add( const std::string & segment ) ;
		// Adds a data segment.

	void add( const char * p , size_t n ) ;
		// Adds a data segment.

	bool more() const ;
		// Returns true if there are complete 
		// line(s) to be extracted.

	const std::string & current() const ;
		// Returns the current line, without extracting 
		// it. The line terminator is not included.
		//
		// Precondition: more()

	void discard() ;
		// Discards the current line.
		//
		// Precondition: more()

	std::string line() ;
		// Extracts a line and returns it as a string. 
		// The line terminator is not included.

private:
	LineBuffer( const LineBuffer & ) ;
	void operator=( const LineBuffer & ) ;
	void load() ;
	void check( size_t ) ;

private:
	static unsigned long m_limit ;
	std::string m_eol ;
	std::string m_current ;
	std::string m_store ;
	bool m_more ;
	bool m_throw ;
} ;

#endif

