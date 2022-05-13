//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_NET_LINE_BUFFER_H
#define G_NET_LINE_BUFFER_H

#include "gdef.h"
#include "gexception.h"
#include "glinestore.h"
#include "gstringview.h"
#include "gcall.h"
#include <functional>
#include <string>

namespace GNet
{
	class LineBuffer ;
	class LineBufferConfig ;
	class LineBufferIterator ;
	class LineBufferState ;
}

//| \class GNet::LineBuffer
/// A class that does line buffering, supporting auto-detection of
/// line endings and fixed-size block extraction. Raw data is
/// added, and newline-delimited lines are extracted, optionally
/// via an iterator.
///
/// Usage:
/// \code
/// {
///   GNet::LineBuffer buffer( (GNet::LineBufferConfig()) ) ;
///   buffer.add("abc") ;
///   buffer.add("def\nABC\nDE") ;
///   buffer.add("F\n") ;
///
///   while( buffer.more() )
///     cout << std::string_view(buffer.data(),buffer.size()) << endl ;
/// }
/// \endcode
///
/// A callback mechanism (apply()) can be used that combines
/// adding and extracting. This has the benefit of less data
/// copying, especially if the caller allows incomplete line
/// fragments to be delivered.
///
/// \code
/// {
///   struct Callback { bool operator()( const char * , std::size_t size , std::size_t eolsize , std::size_t linesize , char c0 ) {...} } callback ;
///   GNet::LineBuffer buffer( (GNet::LineBufferConfig()) ) ;
///   for( std::string s : std::vector<std::string> { "foo" , "bar\r" , "\n" } )
///     buffer.apply( s.data() , s.size() , callback , true ) ;
/// }
/// \endcode
///
/// The expect() method allows for handling fixed-size blocks
/// that are not line-structured (think http content-length).
/// While the expect() value is in force the line buffer is in
/// a transparent mode, delivering data() with a zero eolsize().
///
/// Note that a line buffer that is configured as 'transparent'
/// at run-time is essentially zero cost when using apply()
/// with the 'fragments' option: data passes directly from
/// apply() to the callback.
///
class GNet::LineBuffer
{
public:
	G_EXCEPTION( ErrorOverflow , tx("line buffer overflow") ) ;
	using SinkArgs = std::tuple<const char*,std::size_t,std::size_t,std::size_t,char,bool> ;
	using SinkFn = std::function<bool(const SinkArgs&)> ;
	using FragmentsFn = std::function<bool()> ;

	explicit LineBuffer( const LineBufferConfig & ) ;
		///< Constructor.

	void clear() ;
		///< Clears the internal data.

	void add( const std::string & data ) ;
		///< Adds a data segment.

	void add( const char * data , std::size_t size ) ;
		///< Adds a data segment by copying.
		///< See also apply().

	void expect( std::size_t n ) ;
		///< Requests that the next 'n' bytes are extracted in one
		///< contiguous block, without regard to line endings. Once
		///< the expected number of bytes have been extracted the
		///< line buffering returns to normal.
		///<
		///< This method can be used during a data-transfer phase to
		///< obtain a chunk of data of known size, as in http with a
		///< known content-length.
		///<
		///< A parameter value of zero switches back to normal line
		///< buffering immediately.
		///<
		///< A parameter value of std::size_t(-1) can be used to represent
		///< an infinite expectation that is never fully satisfied.
		///< This is only sensible when extracting fragments with
		///< apply(), and it results in full transparency.

	std::string eol() const ;
		///< Returns the end-of-line string as passed in to the
		///< constructor, or as auto-detected. Returns the empty
		///< string if auto-detection by iteration has not yet
		///< occurred.

	template <typename Tfn>
	void apply( const char * data , std::size_t data_size , Tfn sink_fn , bool fragments = false ) ;
		///< Adds the data and passes complete lines to the sink
		///< function with line-data, line-size, eol-size and
		///< c0 parameters. Stops if the sink function returns false.
		///< The data can be nullptr in order to flush any existing
		///< data to the sink function. This method is zero-copy if
		///< the supplied data contains complete lines or if allowing
		///< line fragments.
		///<
		///< \code
		///< void Foo::onData( const char * data , std::size_t size )
		///< {
		///<   apply( data , size , onLine , false ) ;
		///< }
		///< bool onLine( const char * data , std::size_t size , std::size_t , std::size_t , char )
		///< {
		///<   process( std::string(data,size) ) ;
		///< }
		///< \endcode

	template <typename Tsink, typename Tmemfun>
	void apply( Tsink sink_p , Tmemfun sink_memfun , const char * data , std::size_t data_size ,
		bool fragments = false ) ;
			///< Overload that calls out to a member function.
			///<
			///< \code
			///< void Foo::onData( const char * data , std::size_t size )
			///< {
			///<   apply( this , &Foo::onLine , data , size , false ) ;
			///< }
			///< bool Foo::onLine( const char * data , std::size_t size , std::size_t , std::size_t , char )
			///< {
			///<   process( std::string(data,size) ) ;
			///< }
			///< \endcode

	template <typename Tsink, typename Tmemfun, typename Tmemfun2>
	void apply( Tsink sink_p , Tmemfun sink_memfun , const char * data , std::size_t data_size ,
		Tmemfun2 fragments_memfun ) ;
			///< Overload where the 'fragments' flag comes from calling a member
			///< function on the sink object, allowing the flag to change
			///< dynamically as each line is delivered.

	bool apply( SinkFn sink_fn , G::string_view data , FragmentsFn fragments_fn ) ;
		///< Overload for std::function.
		///<
		///< This overload provides extra parameter 'more' to the
		///< sink function that is true if there is another complete
		///< line buffered-up after the current one (and eolsize is
		///< non-zero).
		///<
		///< Returns false iff the sink function returned false.

	template <typename Tfn>
	void apply( const std::string & , Tfn sink_fn , bool fragments = false ) ;
		///< Overload taking a string as its data input, used in
		///< testing.

	bool more( bool fragments = false ) ;
		///< Returns true if there is more data() to be had.
		///< This advances the implied iterator.
		///<
		///< If the fragments parameter is true then incomplete
		///< lines will be returned (with eolsize zero), but
		///< those fragments will explicitly exclude anything
		///< that might be part of the line ending.

	const char * data() const ;
		///< Returns a pointer for the current line, expect()ed
		///< fixed-size block, or line fragment. This includes
		///< eolsize() bytes of line-ending.
		///< Precondition: more()

	std::size_t size() const ;
		///< Returns the size of the current data(), excluding the
		///< line ending.
		///< Precondition: more()

	std::size_t eolsize() const ;
		///< Returns the size of line-ending associated with the
		///< current data(). This will be zero for a fixed-size
		///< block or non-terminal line fragment. (It will never
		///< be zero as a result of auto-detection because the
		///< precondition means that auto-detection has already
		///< happened.)
		///< Precondition: more()

	std::size_t linesize() const ;
		///< Returns the current size of all the line fragments
		///< making up the current line.
		///< Precondition: more()

	char c0() const ;
		///< Returns the first character of the current line.
		///< This can be useful in the case of line fragments
		///< where treatment of the fragment depends on the
		///< first character of the complete line (as in SMTP
		///< data transfer).
		///< Precondition: linesize() != 0U

	bool transparent() const ;
		///< Returns true if the current expect() value is
		///< infinite.

	LineBufferState state() const ;
		///< Returns information about the current state of the
		///< line-buffer.

	std::size_t buffersize() const ;
		///< Returns the total number of bytes buffered up.

	bool peekmore() const ;
		///< Returns true if there is a line available after the
		///< current line or expect()ation.
		///< Precondition: more()

public:
	void extensionStart( const char * , std::size_t ) ;
		///< A pseudo-private method used by the implementation
		///< of the apply() method template.

	void extensionEnd() ;
		///< A pseudo-private method used by the implementation
		///< of the apply() method template.

private:
	struct Output
	{
		bool m_first{true} ;
		const char * m_data{nullptr} ;
		std::size_t m_size{0U} ;
		std::size_t m_eolsize{0U} ;
		std::size_t m_linesize{0U} ;
		char m_c0{'\0'} ;
		Output() ;
		std::size_t set( LineStore & , std::size_t pos , std::size_t size , std::size_t eolsize ) ;
	} ;
	struct Extension
	{
		Extension( LineBuffer * , const char * , std::size_t ) ;
		~Extension() ;
		Extension( const Extension & ) = delete ;
		Extension( Extension && ) = delete ;
		void operator=( const Extension & ) = delete ;
		void operator=( Extension && ) = delete ;
		bool valid() const ;
		LineBuffer * m_line_buffer ;
		G::CallFrame m_call_frame ;
	} ;

public:
	~LineBuffer() = default ;
	LineBuffer( const LineBuffer & ) = delete ;
	LineBuffer( LineBuffer && ) = delete ;
	void operator=( const LineBuffer & ) = delete ;
	void operator=( LineBuffer && ) = delete ;

private:
	friend class LineBufferState ;
	void output( std::size_t size , std::size_t eolsize , bool = false ) ;
	bool detect() ;
	bool trivial( std::size_t pos ) const ;
	bool finite() const ;
	std::string head() const ; // for LineBufferState

private:
	friend struct Extension ;
	G::CallStack m_call_stack ;
	bool m_auto ;
	std::string m_eol ;
	std::size_t m_warn_limit ;
	std::size_t m_fmin ;
	std::size_t m_expect ;
	bool m_warned ;
	LineStore m_in ;
	Output m_out ;
	std::size_t m_pos ;
} ;

//| \class GNet::LineBufferConfig
/// A configuration structure for GNet::LineBuffer.
///
class GNet::LineBufferConfig
{
public:
	explicit LineBufferConfig( const std::string & eol = std::string(1U,'\n') ,
		std::size_t warn = 0U , std::size_t fmin = 0U , std::size_t initial_expect = 0U ) ;
			///< Constructor. An empty end-of-line string detects either
			///< LF or CR-LF. The default end-of-line string is newline.
			///< A non-zero warn-limit generates a one-shot warning when
			///< breached. The fmin value can be used to prevent trivially
			///< small line fragments from being returned. This is useful
			///< for SMTP where a fragment containing a single dot character
			///< and no end-of-line can cause confusion with respect to
			///< the end-of-text marker. The initial-expect parameter
			///< is useful for defining transparent operation.

	const std::string & eol() const ;
		///< Returns the end-of-line string as passed to the constructor.

	std::size_t warn() const ;
		///< Returns the warn-limit, as passed to the constructor.

	std::size_t fmin() const ;
		///< Returns the minimum fragment size, as passed to the constructor.

	std::size_t expect() const ;
		///< Returns the initial expect value, as passed to the constructor.

	bool operator==( const LineBufferConfig & ) const ;
		///< Equality operator.

	static LineBufferConfig transparent() ;
		///< Convenience factory function.

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
	std::size_t m_warn ;
	std::size_t m_fmin ;
	std::size_t m_expect ;
} ;

//| \class GNet::LineBufferState
/// Provides information abount the state of a line buffer.
///
class GNet::LineBufferState
{
public:
	explicit LineBufferState( const LineBuffer & ) ;
		///< Constructor.

	bool transparent() const ;
		///< Returns LineBuffer::transparent().

	std::string eol() const ;
		///< Returns LineBuffer::eol().

	std::size_t size() const ;
		///< Returns the number of bytes currently buffered up.

	bool empty() const ;
		///< Returns true iff size() is zero.

	std::string head() const ;
		///< Returns the first bytes of buffered data up to a limit
		///< of sixteen bytes.

	bool peekmore() const ;
		///< Returns true if another complete line is available
		///< after the current line or current expect() block.

private:
	bool m_transparent ;
	bool m_peekmore ;
	std::string m_eol ;
	std::size_t m_size ;
	std::string m_head ;
} ;

// ==

inline
GNet::LineBuffer::Extension::Extension( LineBuffer * line_buffer , const char * data , std::size_t size ) :
	m_line_buffer(line_buffer) ,
	m_call_frame(line_buffer->m_call_stack)
{
	m_line_buffer->extensionStart( data , size ) ;
}

inline
GNet::LineBuffer::Extension::~Extension()
{
	if( m_call_frame.valid() )
		m_line_buffer->extensionEnd() ;
}

inline
bool GNet::LineBuffer::Extension::valid() const
{
	return m_call_frame.valid() ;
}

// ==

inline
std::size_t GNet::LineBuffer::size() const
{
	return m_out.m_size ;
}

inline
std::size_t GNet::LineBuffer::buffersize() const
{
	return m_in.size() ;
}

inline
std::string GNet::LineBuffer::head() const
{
	return m_in.head( 16U ) ;
}

inline
std::size_t GNet::LineBuffer::eolsize() const
{
	return m_out.m_eolsize ;
}

inline
std::size_t GNet::LineBuffer::linesize() const
{
	return m_out.m_linesize ;
}

inline
char GNet::LineBuffer::c0() const
{
	return m_out.m_c0 ;
}

template <typename Tfn>
void GNet::LineBuffer::apply( const char * data_in , std::size_t size_in , Tfn sink_fn , bool with_fragments )
{
	Extension e( this , data_in , size_in ) ;
	while( e.valid() && more(with_fragments) )
	{
		if( !sink_fn( data() , size() , eolsize() , linesize() , c0() ) )
			break ;
	}
}

template <typename Tsink, typename Tmemfun>
void GNet::LineBuffer::apply( Tsink sink_p , Tmemfun memfun , const char * data_in , std::size_t size_in , bool with_fragments )
{
	Extension e( this , data_in , size_in ) ;
	while( e.valid() && more(with_fragments) )
	{
		if( !(sink_p->*memfun)( data() , size() , eolsize() , linesize() , c0() ) )
			break ;
	}
}

template <typename Tsink, typename Tmemfun, typename Tmemfun2>
void GNet::LineBuffer::apply( Tsink sink_p , Tmemfun memfun , const char * data_in , std::size_t size_in , Tmemfun2 fragments_memfun )
{
	Extension e( this , data_in , size_in ) ;
	while( e.valid() && more( (sink_p->*fragments_memfun)() ) )
	{
		if( !(sink_p->*memfun)( data() , size() , eolsize() , linesize() , c0() ) )
			break ;
	}
}

inline
bool GNet::LineBuffer::apply( SinkFn sink_fn , G::string_view data_in , FragmentsFn fragments_fn )
{
	Extension e( this , data_in.data() , data_in.size() ) ;
	while( e.valid() && more( fragments_fn() ) )
	{
		if( !sink_fn( SinkArgs(data(),size(),eolsize(),linesize(),c0(),peekmore()) ) )
			return false ;
	}
	return true ;
}

template <typename T>
inline
void GNet::LineBuffer::apply( const std::string & data , T sink , bool with_fragments )
{
	return apply( data.data() , data.size() , sink , with_fragments ) ;
}

// ==

inline
const std::string & GNet::LineBufferConfig::eol() const
{
	return m_eol ;
}

inline
std::size_t GNet::LineBufferConfig::warn() const
{
	return m_warn ;
}

inline
std::size_t GNet::LineBufferConfig::fmin() const
{
	return m_fmin ;
}

inline
std::size_t GNet::LineBufferConfig::expect() const
{
	return m_expect ;
}

// ==

inline
GNet::LineBufferState::LineBufferState( const LineBuffer & line_buffer ) :
	m_transparent(line_buffer.transparent()) ,
	m_peekmore(line_buffer.peekmore()) ,
	m_eol(line_buffer.eol()) ,
	m_size(line_buffer.buffersize()) ,
	m_head(line_buffer.head())
{
}

inline
std::string GNet::LineBufferState::eol() const
{
	return m_eol ;
}

inline
bool GNet::LineBufferState::transparent() const
{
	return m_transparent ;
}

inline
std::size_t GNet::LineBufferState::size() const
{
	return m_size ;
}

inline
bool GNet::LineBufferState::empty() const
{
	return m_size == 0U ;
}

inline
std::string GNet::LineBufferState::head() const
{
	return m_head ;
}

inline
bool GNet::LineBufferState::peekmore() const
{
	return m_peekmore ;
}

#endif
