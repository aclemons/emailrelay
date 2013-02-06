//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
	class LineBufferIterator ;
}

/// \class GNet::LineBuffer
/// A class which does line buffering. Raw data is added, and 
/// newline-delimited lines are extracted.
///
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
	friend class LineBufferIterator ;
	LineBuffer( const LineBuffer & ) ;
	void operator=( const LineBuffer & ) ;
	void fix( std::string::size_type ) ;
	void check( std::string::size_type ) ;
	void lock() ;
	void unlock( std::string::size_type ) ;

private:
	static unsigned long m_limit ;
	std::string m_eol ;
	std::string::size_type m_eol_length ;
	std::string m_store ;
	std::string::size_type m_p ;
	bool m_current_valid ; // mutable
	std::string m_current ; // mutable
	bool m_throw ;
	bool m_locked ;
} ;

/// \class GNet::LineBufferIterator
/// An iterator class for GNet::LineBuffer.
/// Use of this class is optional but it may provide
/// some performance improvement. You are not allowed to add()
/// more data to the underlying line buffer while iterating.
///
class GNet::LineBufferIterator 
{
public:
	explicit LineBufferIterator( LineBuffer & ) ;
		///< Constructor.

	~LineBufferIterator() ;
		///< Destructor.

	bool more() const ;
		///< Returns true if there is a line() to be had.

	const std::string & line() ;
		///< Returns the current line and increments the iterator.
		///< Precondition: more()

private:
	LineBufferIterator( const LineBufferIterator & ) ; // not implemented
	void operator=( const LineBufferIterator & ) ; // not implemented

private:
	LineBuffer & m_b ;
	std::string::size_type m_n ;
	std::string::size_type m_store_length ;
} ;

inline
GNet::LineBufferIterator::LineBufferIterator( LineBuffer & b ) :
	m_b(b) ,
	m_n(0U) ,
	m_store_length(b.m_store.length())
{
	m_b.lock() ;
}

inline
GNet::LineBufferIterator::~LineBufferIterator()
{
	m_b.unlock( m_n ) ;
}

#endif

