//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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

namespace G
{
	template <typename Tchar> class basic_omembuf ;
	using omembuf = basic_omembuf<char> ;
	using womembuf = basic_omembuf<wchar_t> ;
}

//| \class G::basic_omembuf
/// An output streambuf that writes to a fixed-size char buffer.
/// Does not support seeking.
///
/// Eg:
/// \code
/// std::array<char,10> buffer ;
/// G::basic_omembuf<char> sb( buffer.data() , buffer.size() ) ;
/// std::ostream out( &sb ) ;
/// \endcode
///
/// An alternative approach is to use std::ostringstream with
/// pubsetbuf() but there is no guarantee that the std::stringbuf
/// implementation has a useful override of setbuf() (ie. msvc).
///
template <typename Tchar>
class G::basic_omembuf : public std::basic_streambuf<Tchar,std::char_traits<Tchar>>
{
public:
	using base_type = std::basic_streambuf<Tchar,std::char_traits<Tchar>> ;

	basic_omembuf( Tchar * p , std::size_t n ) ;
		///< Constructor.

private: // overrides
	std::streambuf * setbuf( Tchar * p , std::streamsize n ) override ;
		///< Overridden because we can. Called by streambuf::pubsetbuf().

	std::streampos seekoff( std::streamoff , std::ios_base::seekdir , std::ios_base::openmode ) override ;
		///< Overridden with only a partial implementation for tellp(),
		///< so not fully seekable. Called by streambuf::pubseekoff(),
		///< streambuf::tellp() etc.

	std::streampos seekpos( std::streampos , std::ios_base::openmode ) override ;
		///< Overridden with only a partial implementation for seekp(0),
		///< so not fully seekable. Called by streambuf::pubseekpos().

	std::streamsize xsputn( const Tchar * p , std::streamsize n ) override ;
		///< Overridden for efficiency compared to multiple sputc()s.
		///< Called by streambuf::sputn().

public:
	~basic_omembuf() override = default ;
	basic_omembuf( const basic_omembuf<Tchar> & ) = delete ;
	basic_omembuf( basic_omembuf<Tchar> && ) = delete ;
	basic_omembuf<Tchar> & operator=( const basic_omembuf<Tchar> & ) = delete ;
	basic_omembuf<Tchar> & operator=( basic_omembuf<Tchar> && ) = delete ;
} ;

template <typename Tchar>
G::basic_omembuf<Tchar>::basic_omembuf( Tchar * p , std::size_t n )
{
	base_type::setp( p , p+n ) ;
}

template <typename Tchar>
std::streambuf * G::basic_omembuf<Tchar>::setbuf( Tchar * p , std::streamsize n )
{
	base_type::setp( p , p+n ) ;
	return this ;
}

template <typename Tchar>
std::streampos G::basic_omembuf<Tchar>::seekoff( std::streamoff off , std::ios_base::seekdir way , std::ios_base::openmode which )
{
	if( off == 0 && way == std::ios_base::cur && ( which & std::ios_base::out ) ) // NOLINT
		return base_type::pptr() - base_type::pbase() ;
	else
		return -1 ;
}

template <typename Tchar>
std::streampos G::basic_omembuf<Tchar>::seekpos( std::streampos pos , std::ios_base::openmode which )
{
	if( pos == 0 && ( which & std::ios_base::out ) ) // NOLINT
	{
		base_type::setp( base_type::pbase() , base_type::epptr() ) ;
		return 0 ;
	}
	else
	{
		return -1 ;
	}
}

template <typename Tchar>
std::streamsize G::basic_omembuf<Tchar>::xsputn( const Tchar * p , std::streamsize n )
{
	Tchar * start = base_type::pptr() ;
	if( start == nullptr ) return 0 ;
	std::streamsize space = base_type::epptr() - start ;
	std::streamsize ncopy = std::min( space , n ) ;
	std::copy_n( p , static_cast<std::size_t>(ncopy) , start ) ;
	base_type::pbump( static_cast<int>(ncopy) ) ;
	return ncopy ;
}

#endif
