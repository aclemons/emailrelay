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
/// \file gconvert.cpp
///

#include "gdef.h"
#include "gconvert.h"
#include <algorithm>
#include <type_traits>
#include <cwchar> // std::wsclen()

bool G::Convert::m_utf16 = sizeof(wchar_t) == 2 ;

#ifndef G_LIB_SMALL
bool G::Convert::utf16( bool b )
{
	std::swap( m_utf16 , b ) ;
	return b ;
}
#endif

#ifndef G_LIB_SMALL
std::wstring G::Convert::widen( std::string_view sv )
{
	return sv.empty() ? std::wstring() : widenImp( sv.data() , sv.size() ) ;
}
#endif

bool G::Convert::valid( std::string_view sv ) noexcept
{
	if( sv.empty() ) return true ;
	bool valid = true ;
	widenImp( sv.data() , sv.size() , nullptr , &valid ) ;
	return valid ;
}

#ifndef G_LIB_SMALL
std::string G::Convert::narrow( const std::wstring & s )
{
	return s.empty() ? std::string() : narrowImp( s.data() , s.size() ) ;
}
#endif

#ifndef G_LIB_SMALL
std::string G::Convert::narrow( const wchar_t * p )
{
	return p && *p ? narrowImp( p , std::wcslen(p) ) : std::string() ;
}
#endif

#ifndef G_LIB_SMALL
std::string G::Convert::narrow( const wchar_t * p , std::size_t n )
{
	return p && n ? narrowImp( p , n ) : std::string() ;
}
#endif

#ifndef G_LIB_SMALL
bool G::Convert::invalid( const std::wstring & s )
{
	return s.find( L'\xFFFD' ) != std::string::npos ;
}
#endif

#ifndef G_LIB_SMALL
bool G::Convert::invalid( const std::string & s )
{
	return s.find( "\xEF\xBF\xBD" ) != std::string::npos ;
}
#endif

// ==

std::wstring G::Convert::widenImp( const char * p_in , std::size_t n_in )
{
	std::size_t n_out = widenImp( p_in , n_in , nullptr ) ;
	if( n_out == 0U ) return {} ;
	std::wstring out ;
	out.resize( n_out ) ;
	widenImp( p_in , n_in , &*out.begin() ) ;
	return out ;
}

std::size_t G::Convert::widenImp( const char * p_in , std::size_t n_in , wchar_t * p_out , bool * valid_out ) noexcept
{
	const unsigned char * p = reinterpret_cast<const unsigned char*>( p_in ) ;
	std::size_t n_out = 0U ;
	std::size_t d = 0U ;
	for( std::size_t i = 0U ; i < n_in ; i += d , p += d )
	{
		// UTF-8 in
		auto pair = u8in( p , n_in-i ) ;
		unicode_type u = pair.first ;
		d = pair.second ;
		if( u == unicode_error )
		{
			u = 0xFFFD ;
			if( valid_out )
				*valid_out = false ;
		}

		if( m_utf16 ) // UTF-16 out
		{
			if( u <= 0xD7FF || ( u >= 0xE000 && u <= 0xFFFF ) )
			{
				if( p_out )
					*p_out++ = static_cast<wchar_t>(u) ;
				n_out++ ;
			}
			else if( u >= 0x10000 && u <= 0x10FFFF )
			{
				static_assert( (0x10FFFF - 0x10000) == 0xFFFFF , "" ) ;
				u -= 0x10000 ;
				if( p_out )
				{
					*p_out++ = static_cast<wchar_t>( 0xD800 | (u >> 10) ) ;
					*p_out++ = static_cast<wchar_t>( 0xDC00 | (u & 0x3FF) ) ;
				}
				n_out += 2 ;
			}
			else
			{
				if( p_out )
					*p_out++ = L'\xFFFD' ;
				n_out++ ;
			}
		}
		else // UCS-4 out
		{
			if( p_out )
				*p_out++ = static_cast<wchar_t>(u) ;
			n_out++ ;
		}
	}
	return n_out ;
}

std::pair<G::Convert::unicode_type,std::size_t> G::Convert::u8in( const unsigned char * p , std::size_t n ) noexcept
{
	unicode_type u = unicode_error ;
	std::size_t d = 1U ;
	if( n == 0U )
	{
	}
	else if( ( p[0] & 0x80U ) == 0U ) // 0...
	{
		u = p[0] ;
	}
	else if( ( p[0] & 0xC0 ) == 0x80 ) // 10...
	{
	}
	else if( ( p[0] & 0xE0U ) == 0xC0U && n > 1U && // 110...
		!( ( p[0] & 0x1EU ) == 0U ) && // (not overlong)
		( p[1] & 0xC0 ) == 0x80U )
	{
		d = 2U ;
		u = ( static_cast<unicode_type>(p[0]) & 0x1FU ) << 6 ;
		u |= ( static_cast<unicode_type>(p[1]) & 0x3FU ) << 0 ;
	}
	else if( ( p[0] & 0xE0U ) == 0xC0U )
	{
		//d = 2U ;
	}
	else if( ( p[0] & 0xF0U ) == 0xE0U && n > 2U && // 1110...
		!( ( p[0] & 0x0FU ) == 0U && ( p[1] & 0x20U ) == 0U ) && // (not overlong)
		!( ( p[0] & 0x0FU ) == 0x0DU && ( p[1] & 0x20U ) == 0x20U ) && // (not UTF-16 surrogate)
		( p[1] & 0xC0 ) == 0x80U &&
		( p[2] & 0xC0 ) == 0x80U )
	{
		d = 3U ;
		u = ( static_cast<unicode_type>(p[0]) & 0x0FU ) << 12 ;
		u |= ( static_cast<unicode_type>(p[1]) & 0x3FU ) << 6 ;
		u |= ( static_cast<unicode_type>(p[2]) & 0x3FU ) << 0 ;
	}
	else if( ( p[0] & 0xF0U ) == 0xE0U )
	{
		//d = 3U ;
	}
	else if( ( p[0] & 0xF8U ) == 0xF0U && n > 3U &&  // 11110...
		!( ( p[0] & 0x07U ) == 0U && ( p[1] & 0x30U ) == 0U ) && // (not overlong)
		( p[1] & 0xC0 ) == 0x80U &&
		( p[2] & 0xC0 ) == 0x80U &&
		( p[3] & 0xC0 ) == 0x80U )
	{
		d = 4U ;
		u = ( static_cast<unicode_type>(p[0]) & 0x07U ) << 18 ;
		u |= ( static_cast<unicode_type>(p[1]) & 0x3FU ) << 12 ;
		u |= ( static_cast<unicode_type>(p[2]) & 0x3FU ) << 6 ;
		u |= ( static_cast<unicode_type>(p[3]) & 0x3FU ) << 0 ;
	}
	else if( ( p[0] & 0xF8U ) == 0xF0U )
	{
		//d = 4U ;
	}
	else // 11111...
	{
	}
	return std::make_pair( u , d ) ;
}

void G::Convert::u8parse( std::string_view s , ParseFn fn )
{
	const unsigned char * p = reinterpret_cast<const unsigned char*>( s.data() ) ;
	std::size_t n = s.size() ;
	std::size_t d = 0U ;
	for( std::size_t i = 0U ; i < n ; i += d , p += d )
	{
		auto pair = u8in( p , n-i ) ;
		if( pair.first == unicode_error ) pair.first = L'\xFFFD' ;
		if( !fn( pair.first , pair.second , i ) ) break ;
		d = std::max( pair.second , std::size_t(1U) ) ;
	}
}

std::string G::Convert::narrowImp( const wchar_t * p_in , std::size_t n_in )
{
	std::size_t n_out = narrowImp( p_in , n_in , nullptr ) ;
	std::string s ;
	s.resize( n_out ) ;
	G::Convert::narrowImp( p_in , n_in , &*s.begin() ) ;
	return s ;
}

std::size_t G::Convert::narrowImp( const wchar_t * p_in , std::size_t n_in , char * p_out ) noexcept
{
	std::size_t n_out = 0U ;
	std::size_t d = 1U ;
	for( std::size_t i = 0U ; i < n_in ; i += d , p_in += d )
	{
		unicode_type u = unicode_cast( p_in[0] ) ;
		bool error = false ;
		d = 1U ;
		if( m_utf16 ) // UTF-16 in
		{
			auto u1 = unicode_cast( m_utf16 && (i+1U) < n_in ? p_in[1] : L'\0' ) ;
			const bool u0_high = u >= 0xD800U && u <= 0xDBFFU ;
			const bool u0_low = u >= 0xDC00U && u <= 0xDFFFU ;
			const bool u1_low = u1 >= 0xDC00U && u1 <= 0xDFFFU ;
			if( u0_high && u1_low )
			{
				d = 2U ;
				u = 0x10000U | ((u & 0x3FFU) << 10) | (u1 & 0x3FFU) ;
			}
			else if( u0_high || u0_low )
			{
				error = true ;
			}
		}

		// UTF-8 out
		if( error || u > 0x10FFFFU )
		{
			if( p_out )
			{
				*p_out++ = '\xEF' ;
				*p_out++ = '\xBF' ;
				*p_out++ = '\xBD' ;
			}
			n_out += 3 ;
		}
		else
		{
			n_out += u8out( u , p_out ) ;
		}
	}
	return n_out ;
}

std::size_t G::Convert::u8out( unicode_type u , char* & p_out ) noexcept
{
	if( u > 0x10FFFFU )
	{
		return 0U ;
	}
	else if( u <= 0x7FU )
	{
		if( p_out )
			*p_out++ = static_cast<unsigned char>(u) ; // NOLINT
		return 1U ;
	}
	else if( u >= 0x80U && u <= 0x7FFU )
	{
		if( p_out )
		{
			*p_out++ = char_cast( 0xC0U | ((u >> 6) & 0x1FU) ) ;
			*p_out++ = char_cast( 0x80U | ((u >> 0) & 0x3FU) ) ;
		}
		return 2U ;
	}
	else if( u >= 0x800U && u <= 0xFFFFU )
	{
		if( p_out )
		{
			*p_out++ = char_cast( 0xE0U | ((u >> 12) & 0x0FU) ) ;
			*p_out++ = char_cast( 0x80U | ((u >> 6) & 0x3FU) ) ;
			*p_out++ = char_cast( 0x80U | ((u >> 0) & 0x3FU) ) ;
		}
		return 3U ;
	}
	else
	{
		if( p_out )
		{
			*p_out++ = char_cast( 0xF0U | ((u >> 18) & 0x07U) ) ;
			*p_out++ = char_cast( 0x80U | ((u >> 12) & 0x3FU) ) ;
			*p_out++ = char_cast( 0x80U | ((u >> 6) & 0x3FU) ) ;
			*p_out++ = char_cast( 0x80U | ((u >> 0) & 0x3FU) ) ;
		}
		return 4U ;
	}
}

G::Convert::unicode_type G::Convert::unicode_cast( wchar_t c ) noexcept
{
	return static_cast<unicode_type>( static_cast<std::make_unsigned<wchar_t>::type>(c) ) ; // NOLINT clang-tidy confusion
}

char G::Convert::char_cast( unsigned int i ) noexcept
{
	return static_cast<char>( static_cast<unsigned char>( i ) ) ; // NOLINT narrowing
}

