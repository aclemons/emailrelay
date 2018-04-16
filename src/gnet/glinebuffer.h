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
/// \file glinebuffer.h
///

#ifndef G_NET_LINE_BUFFER__H
#define G_NET_LINE_BUFFER__H

#include "gdef.h"
#include "gexception.h"
#include <string>

namespace GNet
{
	class LineBuffer ;
	class LineBufferIterator ;
}

/// \class GNet::LineBuffer
/// A class which does line buffering. Raw data is added, and newline-delimited
/// lines are extracted via an iterator.
///
/// Usage:
/// \code
/// {
///   GNet::LineBuffer buffer ;
///   buffer.add("abc") ;
///   buffer.add("def\nABC\nDE") ;
///   buffer.add("F\n") ;
///   GNet::LineBufferIterator iter( buffer ) ;
///   while( iter.more() )
///     cout << iter.line() << endl ;
/// }
/// \endcode
///
class GNet::LineBuffer
{
public:
	G_EXCEPTION( Error , "line buffer error" ) ;

	LineBuffer() ;
		///< Default constructor for a line buffer that auto-detects either
		///< CR or CR-LF line endings based on the first line.

	explicit LineBuffer( const std::string & eol , bool do_throw_on_overflow = false ) ;
		///< Constructor. The default is to not throw on overflow because
		///< the very large overflow limit is only intended to be protection
		///< against a rogue client or a denial-of-service attack.

	void add( const std::string & segment ) ;
		///< Adds a data segment.

	void add( const char * p , std::string::size_type n ) ;
		///< Adds a data segment.

	const std::string & eol() const ;
		///< Returns the line-ending.

	void expect( size_t n ) ;
		///< The next 'n' bytes added and/or extracted are treated as a
		///< complete line. This is useful for binary chunks of known
		///< size surrounded by text, as in http.

private:
	friend class LineBufferIterator ;
	LineBuffer( const LineBuffer & ) ;
	void operator=( const LineBuffer & ) ;
	size_t lock( LineBufferIterator * ) ;
	void unlock( LineBufferIterator * , size_t , size_t ) ;
	bool check( size_t ) const ;
	void detect() ;

private:
	LineBufferIterator * m_iterator ;
	bool m_auto ;
	std::string m_eol ;
	bool m_throw_on_overflow ;
	std::string m_store ;
	size_t m_expect ;
} ;

/// \class GNet::LineBufferIterator
/// An iterator class for GNet::LineBuffer that extracts complete lines.
/// Iteration and add()ing should not be mixed.
///
class GNet::LineBufferIterator
{
public:
	explicit LineBufferIterator( LineBuffer & ) ;
		///< Constructor.

	~LineBufferIterator() ;
		///< Destructor.

	bool more() ;
		///< Returns true if there is a line() to be had.

	const std::string & line() const ;
		///< Returns the current line.
		///< Precondition: more()

	std::string::const_iterator begin() const ;
		///< Returns a begin iterator for the current line.
		///< Precondition: more()

	std::string::const_iterator end() const ;
		///< Returns an end iterator for the current line.
		///< Precondition: more()

private:
	friend class LineBuffer ;
	LineBufferIterator( const LineBufferIterator & ) ;
	void operator=( const LineBufferIterator & ) ;
	void expect( size_t ) ;

private:
	LineBuffer & m_buffer ;
	size_t m_expect ;
	std::string::size_type m_pos ;
	std::string::size_type m_eol_size ;
	std::string::const_iterator m_line_begin ;
	std::string::const_iterator m_line_end ;
	mutable std::string m_line ;
	mutable bool m_line_valid ;
} ;

#endif
