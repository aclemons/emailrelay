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
/// \file gimembuf.h
///

#ifndef G_IMEMBUF_H
#define G_IMEMBUF_H

#include "gdef.h"
#include <streambuf>
#include <algorithm>

namespace G
{
	template <typename Tchar> class basic_imembuf ;
	using imembuf = basic_imembuf<char> ;
	using wimembuf = basic_imembuf<wchar_t> ;
}

//| \class G::basic_imembuf
/// An input streambuf that takes its data from a fixed-size const
/// buffer.
///
/// Eg:
/// \code
/// std::array<char,10> buffer ;
/// G::basic_imembuf<char> sb( buffer.data() , buffer.size() ) ;
/// std::istream in( &sb ) ;
/// \endcode
///
/// An alternative approach is to use std::istringstream with
/// pubsetbuf() but there is no guarantee that the std::stringbuf
/// implementation has a useful override of setbuf() (ie. msvc).
///
template <typename Tchar>
class G::basic_imembuf : public std::basic_streambuf<Tchar>
{
public:
	basic_imembuf( const Tchar * p , std::size_t n ) ;
		///< Constructor.

protected:
	std::streamsize xsgetn( Tchar * s , std::streamsize n ) override ;
	std::streampos seekpos( std::streampos pos , std::ios_base::openmode which ) override ;
	std::streampos seekoff( std::streamoff off , std::ios_base::seekdir way , std::ios_base::openmode which ) override ;

public:
	~basic_imembuf() override = default ;
	basic_imembuf( const basic_imembuf<Tchar> & ) = delete ;
	basic_imembuf( basic_imembuf<Tchar> && ) = delete ;
	basic_imembuf<Tchar> & operator=( const basic_imembuf<Tchar> & ) = delete ;
	basic_imembuf<Tchar> & operator=( basic_imembuf<Tchar> && ) = delete ;

private:
	template <typename Tint> static std::streamsize min( Tint a , std::streamsize b ) ;

private:
	const Tchar * m_p ;
	std::size_t m_n ;
} ;

template <typename Tchar>
template <typename Tint> std::streamsize G::basic_imembuf<Tchar>::min( Tint a , std::streamsize b )
{
	return std::min( static_cast<std::streamsize>(a) , b ) ;
}

template <typename Tchar>
G::basic_imembuf<Tchar>::basic_imembuf( const Tchar * p_in , std::size_t n ) :
	m_p(p_in) ,
	m_n(n)
{
	using base_t = std::basic_streambuf<Tchar> ;
	Tchar * mp = const_cast<Tchar*>(m_p) ;
	base_t::setg( mp , mp , mp+m_n ) ;
}

template <typename Tchar>
std::streamsize G::basic_imembuf<Tchar>::xsgetn( Tchar * s , std::streamsize n_in )
{
	using base_t = std::basic_streambuf<Tchar> ;
	Tchar * gp = base_t::gptr() ;
	std::streamsize n = min( m_p+m_n-gp , n_in ) ;
	if( n > 0 )
	{
		std::copy( gp , gp+n , s ) ;
		base_t::gbump( static_cast<int>(n) ) ;
	}
	return n ;
}

template <typename Tchar>
std::streampos G::basic_imembuf<Tchar>::seekpos( std::streampos pos_in , std::ios_base::openmode which )
{
	using base_t = std::basic_streambuf<Tchar> ;
	if( ( which & std::ios_base::in ) && pos_in > 0 ) // NOLINT
	{
		Tchar * mp = const_cast<Tchar*>(m_p) ;
		int pos = static_cast<int>(pos_in) ;
		Tchar * p = std::min( mp+pos , mp+m_n ) ;
		base_t::setg( mp , p , mp+m_n ) ;
		return p - mp ;
	}
	else
	{
		return -1 ;
	}
}

template <typename Tchar>
std::streampos G::basic_imembuf<Tchar>::seekoff( std::streamoff off , std::ios_base::seekdir way , std::ios_base::openmode which )
{
	using base_t = std::basic_streambuf<Tchar> ;
	if( which & std::ios_base::in )
	{
		Tchar * gp = base_t::gptr() ;
		Tchar * mp = const_cast<Tchar*>(m_p) ;
		if( way == std::ios_base::beg )
			gp = std::max( mp , std::min(mp+off,mp+m_n) ) ;
		else if( way == std::ios_base::cur )
			gp = std::max( mp , std::min(gp+off,mp+m_n) ) ;
		else // end
			gp = std::max( mp , std::min(mp+m_n+off,mp+m_n) ) ;
		base_t::setg( mp , gp , mp+m_n ) ;
		return std::streamoff(gp-mp) ;
	}
	else
	{
		return -1 ;
	}
}

#endif
