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
	class LineBufferConfig ;
	class LineBufferIterator ;
}

/// \class GNet::LineBuffer
/// A class that does line buffering, supporting auto-detection of
/// line endings and fixed-size extraction. Raw data is added, and
/// newline-delimited lines are extracted, optionally via an iterator.
///
/// Usage:
/// \code
/// {
///   GNet::LineBuffer buffer( (GNet::LineBufferConfig()) ) ;
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
	G_EXCEPTION( ErrorOverflow , "line buffer overflow" ) ;

	explicit LineBuffer( LineBufferConfig ) ;
		///< Constructor.

	void add( const std::string & data ) ;
		///< Adds a data segment.

	void add( const char * data , size_t size ) ;
		///< Adds a data segment.

	void expect( size_t n ) ;
		///< The next 'n' bytes extracted will be extracted in one
		///< contiguous block, without regard to line endings.
		///<
		///< This method can be used during a data-transfer phase to
		///< obtain a chunk of data of known size, as in http with a
		///< specific content-length.

	std::string eol() const ;
		///< Returns the end-of-line string as passed in to the
		///< constructor, or as auto-detected. Returns the empty
		///< string if auto-detection by iteration has not yet
		///< occurred.

	template <typename Tsink>
	void apply( const char * data , size_t data_size , Tsink sink ) ;
		///< Adds the data and passes complete lines to the sink
		///< functor with line-data, line-size and eol-size parameters.
		///< Stops if the sink functor returns false. This method
		///< is zero-copy if the supplied data contains several
		///< complete lines.

	bool more() ;
		///< Returns true if there is lineData() to be had.

	const char * lineData() const ;
		///< Returns a pointer for the current line, including the
		///< line ending.
		///< Precondition: more()

	size_t lineSize() const ;
		///< Returns the size of the current line, excluding the
		///< line ending.
		///< Precondition: more()

	size_t eolSize() const ;
		///< Returns the size of current line ending. This will
		///< be zero if extracting a fixed-size block. (It will
		///< never be zero as a result of auto-detection because
		///< auto-detection has already happened.)
		///< Precondition: more()

private:
	LineBuffer( const LineBuffer & ) ;
	void operator=( const LineBuffer & ) ;
	void precheck( size_t ) ;
	void linecheck( size_t ) ;
	bool detect() ;
	void addextra( const char * , size_t ) ;
	void consolidate() ;
	const char * extraeol() const ;

private:
	bool m_auto ;
	std::string m_eol ;
	size_t m_warn_limit ;
	size_t m_fail_limit ;
	std::string m_store ;
	const char * m_extra_data ;
	size_t m_extra_size ;
	size_t m_expect ;
	bool m_warned ;
	size_t m_pos ;
	const char * m_line_data ;
	size_t m_line_size ;
	size_t m_eol_size ;
} ;

/// \class GNet::LineBufferIterator
/// Syntactic sugar for calling GNet::LineBuffer iteration methods.
///
class GNet::LineBufferIterator
{
public:
	explicit LineBufferIterator( LineBuffer & buffer ) ;
		///< Constructor.

	bool more() ;
		///< See LineBuffer::more().

	const char * lineData() const ;
		///< See LineBuffer::lineData().

	size_t lineSize() const ;
		///< See LineBuffer::lineSize().

	size_t eolSize() const ;
		///< See LineBuffer::eolSize().

	std::string line() const ;
		///< Returns the current line.

	std::string eol() const ;
		///< Returns the current line ending but returns the empty
		///< string if currently inside an expect() block.

private:
	LineBufferIterator( const LineBufferIterator & ) ;
	void operator=( const LineBufferIterator & ) ;

private:
	LineBuffer & m_buffer ;
} ;

/// \class GNet::LineBufferConfig
/// A configuration structure for GNet::LineBuffer.
///
class GNet::LineBufferConfig
{
public:
	explicit LineBufferConfig( const std::string & eol = std::string(1U,'\n') ,
		size_t warn = 0U , size_t fail = 0U ) ;
			///< Constructor. An empty end-of-line string detects either
			///< LF or CR-LF. The default end-of-line string is newline.
			///< A non-zero warn-limit generates a one-shot warning when
			///< breached, and the fail-limit causes an exception when
			///< breached. The fail-limit defaults to something large if
			///< given as zero.

	const std::string & eol() const ;
		///< Returns the end-of-line string as passed to the constructor.

	size_t warn() const ;
		///< Returns the warn-limit, as passed to the constructor.

	size_t fail() const ;
		///< Returns the fail-limit, as passed to the constructor.

	static LineBufferConfig http() ;
		///< Convenience factory function.

	static LineBufferConfig smtp() ;
		///< Convenience factory function.

	static LineBufferConfig pop() ;
		///< Convenience factory function.

	static LineBufferConfig crlf() ;
		///< Convenience factory function.

	static LineBufferConfig newline() ;
		///< Convenience factory function.

	static LineBufferConfig autodetect() ;
		///< Convenience factory function.

private:
	std::string m_eol ;
	size_t m_warn ;
	size_t m_fail ;
} ;

inline
const std::string & GNet::LineBufferConfig::eol() const
{
	return m_eol ;
}

inline
size_t GNet::LineBufferConfig::warn() const
{
	return m_warn ;
}

inline
size_t GNet::LineBufferConfig::fail() const
{
	return m_fail ;
}

namespace GNet
{
	template <typename T>
	void LineBuffer::apply( const char * data , size_t size , T sink )
	{
		addextra( data , size ) ;
		while( more() )
		{
			if( !sink( lineData() , lineSize() , eolSize() ) )
				break ;
		}
	}
}

inline
GNet::LineBufferIterator::LineBufferIterator( LineBuffer & buffer ) :
	m_buffer(buffer)
{
}

inline
bool GNet::LineBufferIterator::more()
{
	return m_buffer.more() ;
}

inline
std::string GNet::LineBufferIterator::line() const
{
	return std::string( m_buffer.lineData() , m_buffer.lineSize() ) ;
}

inline
std::string GNet::LineBufferIterator::eol() const
{
	return m_buffer.eolSize() == 0U ? std::string() : m_buffer.eol() ;
}

inline
const char * GNet::LineBufferIterator::lineData() const
{
	return m_buffer.lineData() ;
}

inline
size_t GNet::LineBufferIterator::lineSize() const
{
	return m_buffer.lineSize() ;
}

inline
size_t GNet::LineBufferIterator::eolSize() const
{
	return m_buffer.eolSize() ;
}

#endif
