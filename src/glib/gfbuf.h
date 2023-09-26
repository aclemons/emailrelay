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
/// \file gfbuf.h
///

#ifndef G_FBUF_H
#define G_FBUF_H

#include "gdef.h"
#include <streambuf>
#include <functional>
#include <vector>

namespace G
{
	template <typename T, int N=1024> class fbuf ;
}

//| \class G::fbuf
/// A simple file streambuf using a file descriptor and three
/// function pointers for read, write and close operations.
/// The file descriptor type is templated to allow for non-integer
/// file descriptors, such as std::FILE. Does not support seeking.
///
/// Eg:
/// \code
/// G::fbuf<int,1024> fbuf( ::open("temp.out",O_WRONLY) , ::read , ::write , ::close ) ;
/// std::ostream stream( &fbuf ) ;
/// if( fbuf.file() < 0 )
///     stream.clear( std::ios_base::failbit ) ;
/// stream << "hello, world!\n" ;
/// \endcode
///
/// The implementation specialises std::streambuf, overriding
/// overflow(), underflow() and sync() to operate the internal
/// character buffer and file descriptor.
///
template <typename T, int N>
class G::fbuf : public std::streambuf
{
public:
	using read_fn_t = std::function<ssize_t(T,char*,std::size_t)> ;
	using write_fn_t = std::function<ssize_t(T,const char*,std::size_t)> ;
	using close_fn_t = std::function<void(T)> ;

	explicit fbuf( read_fn_t , write_fn_t , close_fn_t ) ;
		///< Constructor. Use open() to initialise.

	explicit fbuf( T file , read_fn_t , write_fn_t , close_fn_t ) ;
		///< Constructor passed an open file descriptor.

	~fbuf() override ;
		///< Destructor. Closes the file.

	void open( T file ) ;
		///< Installs the given file descriptor.

	T file() const ;
		///< Returns the current file descriptor.

protected:
	int overflow( int c ) override ;
		///< Called to put a character into the output buffer.

	int underflow() override ;
		///< Called to pull a character out of the input buffer,
		///< and pre-fill the input buffer if necessary.

	int sync() override ;
		///< Called to sync the stream.

public:
	fbuf( const fbuf<T,N> & ) = delete ;
	fbuf( fbuf<T,N> && ) = delete ;
	fbuf<T,N> & operator=( const fbuf<T,N> & ) = delete ;
	fbuf<T,N> & operator=( fbuf<T,N> && ) = delete ;

private:
	using traits_type = std::streambuf::traits_type ;
	void close() ;

private:
	static constexpr int sync_ok = 0 ;
	static constexpr int sync_fail = -1 ;
	read_fn_t m_read_fn ;
	write_fn_t m_write_fn ;
	close_fn_t m_close_fn ;
	std::vector<char> m_input ;
	std::vector<char> m_output ;
	bool m_file_open ;
	T m_file ;
} ;

template <typename T,int N>
G::fbuf<T,N>::fbuf( G::fbuf<T,N>::read_fn_t read , G::fbuf<T,N>::write_fn_t write , G::fbuf<T,N>::close_fn_t close ) :
	m_read_fn(read) ,
	m_write_fn(write) ,
	m_close_fn(close) ,
	m_input(static_cast<std::size_t>(N)) ,
	m_output(static_cast<std::size_t>(N)) ,
	m_file_open(false) ,
	m_file()
{
}

template <typename T,int N>
G::fbuf<T,N>::fbuf( T file , G::fbuf<T,N>::read_fn_t read , G::fbuf<T,N>::write_fn_t write , G::fbuf<T,N>::close_fn_t close ) :
	m_read_fn(read) ,
	m_write_fn(write) ,
	m_close_fn(close) ,
	m_input(static_cast<std::size_t>(N)) ,
	m_output(static_cast<std::size_t>(N)) ,
	m_file_open(false) ,
	m_file()
{
	open( file ) ;
}

namespace G
{
	template <typename T,int N>
	fbuf<T,N>::~fbuf()
	{
		close() ;
	}
}

template <typename T,int N>
void G::fbuf<T,N>::open( T file )
{
	close() ;

	m_file = file ;
	m_file_open = true ;

	char * input_begin = &m_input[0] ;
	setg( input_begin , input_begin , input_begin ) ;

	char * output_begin = &m_output[0] ;
	char * output_end = output_begin + m_output.size() ;
	setp( output_begin , output_end-1 ) ;
}

template <typename T,int N>
void G::fbuf<T,N>::close()
{
	if( m_file_open )
	{
		sync() ;
		m_close_fn( m_file ) ;
		m_file_open = false ;
	}
}

template <typename T,int N>
int G::fbuf<T,N>::overflow( int c )
{
	if( !traits_type::eq_int_type( c , traits_type::eof() ) )
	{
		*pptr() = traits_type::to_char_type( c ) ;
		pbump( 1 ) ;
	}
	return sync() == sync_fail ? traits_type::eof() : traits_type::not_eof(c) ;
}

template <typename T,int N>
int G::fbuf<T,N>::sync()
{
	if( pbase() == pptr() )
	{
		return sync_ok ;
	}
	else
	{
		std::size_t size = pptr() - pbase() ;
		ssize_t nwrite = m_write_fn( m_file , pbase() , size ) ;
		if( nwrite <= 0 )
		{
			return sync_fail ;
		}
		else if( static_cast<std::size_t>(nwrite) < size )
		{
			std::copy( pbase()+nwrite , pptr() , pbase() ) ;
			setp( pbase() , epptr() ) ; // reset pptr()
			pbump( static_cast<int>(size-nwrite) ) ;
			return sync_fail ;
		}
		else
		{
			setp( pbase() , epptr() ) ;
			return sync_ok ;
		}
	}
}

template <typename T,int N>
int G::fbuf<T,N>::underflow()
{
	if( gptr() == egptr() )
	{
		char * input_begin = &m_input[0] ;
		ssize_t nread = m_read_fn( m_file , input_begin , m_input.size() ) ;
		if( nread <= 0 )
			return traits_type::eof() ;
		std::size_t nreadu = nread >= 0 ? static_cast<std::size_t>(nread) : std::size_t(0U) ;
		setg( input_begin , input_begin , input_begin+nreadu ) ;
	}
	return traits_type::to_int_type( *gptr() ) ;
}

template <typename T,int N>
T G::fbuf<T,N>::file() const
{
	return m_file ;
}

#endif
