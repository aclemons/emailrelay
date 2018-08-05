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
#include "glinestore.h"
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
		///< Adds a data segment by copying.
		///< See also apply().

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
	void apply( const char * data , size_t data_size , Tsink sink , bool fragments = false ) ;
		///< Adds the data and passes complete lines to the sink
		///< functor with line-data, line-size and eol-size parameters.
		///< Stops if the sink functor returns false. This method
		///< is zero-copy if the supplied data contains several
		///< complete lines or if allowing line fragments.

	template <typename Tsink>
	void apply( const std::string & , Tsink sink , bool fragments = false ) ;
		///< String overload, used in testing.

	bool more( bool fragments = false ) ;
		///< Returns true if there is data() to be had.
		///<
		///< If the fragments parameter is true then incomplete
		///< lines will be returned (with eolsize zero), but
		///< those fragments will not include anything that
		///< might be part of the line ending.

	const char * data() const ;
		///< Returns a pointer for the current line, expect()ed
		///< fixed-size block, or line fragment. This includes
		///< eolsize() bytes of line-ending.
		///< Precondition: more()

	size_t size() const ;
		///< Returns the size of the current data(), excluding the
		///< line ending.
		///< Precondition: more()

	size_t eolsize() const ;
		///< Returns the size of line-ending associated with the
		///< current data(). This will be zero for a fixed-size
		///< block or line fragment. (It will never be zero as a
		///< result of auto-detection because auto-detection has
		///< already happened.)
		///< Precondition: more()

	size_t linesize() const ;
		///< Returns the size of all the line fragments making
		///< up the current line.
		///< Precondition: more()

	char c0() const ;
		///< Returns the first character of the current line.
		///< Precondition: linesize() != 0U

	void extensionStart( const char * , size_t ) ;
		///< A pseudo-private method used by the implementation
		///< of the apply() method template.

	void extensionEnd() ;
		///< A pseudo-private method used by the implementation
		///< of the apply() method template.

private:
	struct Output
	{
		bool m_first ;
		const char * m_data ;
		size_t m_size ;
		size_t m_eolsize ;
		size_t m_linesize ;
		char m_c0 ;
		Output() ;
		size_t set( LineStore & , size_t pos , size_t size , size_t eolsize ) ;
	} ;
	struct Extension
	{
		Extension( LineBuffer & , const char * , size_t ) ;
		~Extension() ;
		LineBuffer & m_line_buffer ;
	} ;

private:
	LineBuffer( const LineBuffer & ) ;
	void operator=( const LineBuffer & ) ;
	void check( const Output & ) ;
	void output( size_t size , size_t eolsize , bool = false ) ;
	bool detect() ;
	bool trivial( size_t pos ) const ;

private:
	bool m_auto ;
	std::string m_eol ;
	size_t m_warn_limit ;
	size_t m_fmin ;
	size_t m_expect ;
	bool m_warned ;
	LineStore m_in ;
	Output m_out ;
	size_t m_pos ;
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

	const char * data() const ;
		///< See LineBuffer::data().

	size_t size() const ;
		///< See LineBuffer::size().

	size_t eolsize() const ;
		///< See LineBuffer::eolsize().

	std::string line() const ;
		///< Returns the current line (of length size()).

	std::string eol() const ;
		///< Returns the configured line-ending, but returns the empty
		///< string if currently inside an expect() block.

private:
	LineBufferIterator( const LineBufferIterator & ) ;
	void operator=( const LineBufferIterator & ) ;

private:
	LineBuffer & m_line_buffer ;
} ;

/// \class GNet::LineBufferConfig
/// A configuration structure for GNet::LineBuffer.
///
class GNet::LineBufferConfig
{
public:
	explicit LineBufferConfig( const std::string & eol = std::string(1U,'\n') ,
		size_t warn = 0U , size_t fmin = 0U ) ;
			///< Constructor. An empty end-of-line string detects either
			///< LF or CR-LF. The default end-of-line string is newline.
			///< A non-zero warn-limit generates a one-shot warning when
			///< breached, and the fail-limit causes an exception when
			///< breached. The fmin value can be used to prevent trivially
			///< small line fragments from being returned. This is useful
			///< for SMTP where a fagment containing a single dot character
			///< and no end-of-line can cause confusion with respect to
			///< the end-of-text marker.

	const std::string & eol() const ;
		///< Returns the end-of-line string as passed to the constructor.

	size_t warn() const ;
		///< Returns the warn-limit, as passed to the constructor.

	size_t fmin() const ;
		///< Returns the minimum fragment size, as passed to the constructor.

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
	size_t m_fmin ;
} ;

// ==

inline
GNet::LineBuffer::Extension::Extension( LineBuffer & line_buffer , const char * data , size_t size ) :
	m_line_buffer(line_buffer)
{
	m_line_buffer.extensionStart( data , size ) ;
}

inline
GNet::LineBuffer::Extension::~Extension()
{
	m_line_buffer.extensionEnd() ;
}

// ==

inline
size_t GNet::LineBuffer::size() const
{
	return m_out.m_size ;
}

inline
size_t GNet::LineBuffer::eolsize() const
{
	return m_out.m_eolsize ;
}

inline
size_t GNet::LineBuffer::linesize() const
{
	return m_out.m_linesize ;
}

inline
char GNet::LineBuffer::c0() const
{
	return m_out.m_c0 ;
}

namespace GNet
{
	template <typename T>
	void LineBuffer::apply( const char * data_in , size_t size_in , T sink , bool with_fragments )
	{
		Extension e( *this , data_in , size_in ) ;
		while( more(with_fragments) )
		{
			if( !sink( data() , size() , eolsize() , linesize() , c0() ) )
				break ;
		}
	}
	template <typename T>
	inline
	void LineBuffer::apply( const std::string & data , T sink , bool with_fragments )
	{
		return apply( data.data() , data.size() , sink , with_fragments ) ;
	}
}

// ==

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
size_t GNet::LineBufferConfig::fmin() const
{
	return m_fmin ;
}

// ==

inline
GNet::LineBufferIterator::LineBufferIterator( LineBuffer & line_buffer ) :
	m_line_buffer(line_buffer)
{
}

inline
bool GNet::LineBufferIterator::more()
{
	return m_line_buffer.more() ;
}

inline
std::string GNet::LineBufferIterator::line() const
{
	return std::string( m_line_buffer.data() , m_line_buffer.size() ) ;
}

inline
std::string GNet::LineBufferIterator::eol() const
{
	return m_line_buffer.eolsize() == 0U ? std::string() : m_line_buffer.eol() ;
}

inline
const char * GNet::LineBufferIterator::data() const
{
	return m_line_buffer.data() ;
}

inline
size_t GNet::LineBufferIterator::size() const
{
	return m_line_buffer.size() ;
}

inline
size_t GNet::LineBufferIterator::eolsize() const
{
	return m_line_buffer.eolsize() ;
}

#endif
