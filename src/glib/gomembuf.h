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
/// \file gomembuf.h
///

#ifndef G_OMEMBUF_H
#define G_OMEMBUF_H

#include "gdef.h"
#include <streambuf>
#include <algorithm>
#include <cstring>

namespace G
{
	class omembuf ;
}

//| \class G::omembuf
/// An output streambuf that writes to a fixed-size char buffer.
/// Does not support seeking.
///
/// Eg:
/// \code
/// std::array<char,10> buffer ;
/// G::omembuf sb( &buffer[0] , buffer.size() ) ;
/// std::ostream out( &sb ) ;
/// \endcode
///
/// An alternative approach is to use std::ostringstream with
/// pubsetbuf() but there is no guarantee that the std::stringbuf
/// implementation has a useful override of setbuf() (ie. msvc).
///
class G::omembuf : public std::streambuf
{
public:
	omembuf( char * p , std::size_t n ) ;
		///< Constructor.

private: // overrides
	std::streambuf * setbuf( char * p , std::streamsize n ) override ;
		///< Overridden because we can. Called by streambuf::pubsetbuf().

	std::streampos seekoff( std::streamoff , std::ios_base::seekdir , std::ios_base::openmode ) override ;
		///< Overridden with only a partial implementation for tellp(),
		///< so not fully seekable. Called by streambuf::pubseekoff(),
		///< streambuf::tellp() etc.

	std::streampos seekpos( std::streampos , std::ios_base::openmode ) override ;
		///< Overridden with only a partial implementation for seekp(0),
		///< so not fully seekable. Called by streambuf::pubseekpos().

	std::streamsize xsputn( const char * p , std::streamsize n ) override ;
		///< Overridden for efficiency compared to multiple sputc()s.
		///< Called by streambuf::sputn().

public:
	~omembuf() override = default ;
	omembuf( const omembuf & ) = delete ;
	omembuf( omembuf && ) = delete ;
	omembuf & operator=( const omembuf & ) = delete ;
	omembuf & operator=( omembuf && ) = delete ;
} ;

inline
G::omembuf::omembuf( char * p , std::size_t n )
{
	setp( p , p+n ) ;
}

inline
std::streambuf * G::omembuf::setbuf( char * p , std::streamsize n )
{
	setp( p , p+n ) ;
	return this ;
}

inline
std::streampos G::omembuf::seekoff( std::streamoff off , std::ios_base::seekdir way , std::ios_base::openmode which )
{
	if( off == 0 && way == std::ios_base::cur && ( which & std::ios_base::out ) )
		return pptr() - pbase() ;
	else
		return -1 ;
}

inline
std::streampos G::omembuf::seekpos( std::streampos pos , std::ios_base::openmode which )
{
	if( pos == 0 && ( which & std::ios_base::out ) )
	{
		setp( pbase() , epptr() ) ;
		return 0 ;
	}
	else
	{
		return -1 ;
	}
}

inline
std::streamsize G::omembuf::xsputn( const char * p , std::streamsize n )
{
	char * start = pptr() ;
	if( start == nullptr ) return 0 ;
	std::streamsize space = epptr() - start ;
	std::streamsize ncopy = std::min( space , n ) ;
	std::memcpy( start , p , static_cast<std::size_t>(ncopy) ) ;
	pbump( static_cast<int>(ncopy) ) ;
	return ncopy ;
}

#endif
