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
/// \file gstringview.cpp
///

#include "gdef.h"
#include "gstringview.h"
#include <algorithm>
#include <iterator> // std::distance

namespace G
{
	namespace StringViewImp
	{
		template <typename Tsv>
		bool sv_imatch( Tsv a , Tsv b ) noexcept
		{
			if( a.empty() || b.empty() ) return a.empty() && b.empty() ;
			if( a.size() != b.size() ) return false ;
			const char * const end = a.data() + a.size() ;
			const char * q = b.data() ;
			for( const char * p = a.data() ; p != end ; ++p , ++q )
			{
				char c1 = *p ;
				char c2 = *q ;
				if( c1 >= 'A' && c1 <= 'Z' ) c1 += '\x20' ;
				if( c2 >= 'A' && c2 <= 'Z' ) c2 += '\x20' ;
				if( c1 != c2 ) return false ;
			}
			return true ;
		}
		template <typename Tsv>
		Tsv sv_substr( Tsv sv , std::size_t pos , std::size_t count ) noexcept
		{
			if( pos >= sv.size() )
				return { sv.data() , std::size_t(0U) } ;
			else
				return { sv.data() + pos , std::min(sv.size()-pos,count) } ;
		}
	}
}

#if GCONFIG_HAVE_CXX_STRING_VIEW

bool G::sv_imatch( std::string_view a , std::string_view b ) noexcept
{
	return StringViewImp::sv_imatch<std::string_view>( a , b ) ;
}

std::string_view G::sv_substr_noexcept( std::string_view a , std::size_t pos , std::size_t count ) noexcept
{
	return StringViewImp::sv_substr<std::string_view>( a , pos , count ) ;
}

#else

bool G::sv_imatch( G::string_view a , G::string_view b ) noexcept
{
	return StringViewImp::sv_imatch<G::string_view>( a , b ) ;
}

G::string_view G::sv_substr_noexcept( G::string_view a , std::size_t pos , std::size_t count ) noexcept
{
	return StringViewImp::sv_substr<G::string_view>( a , pos , count ) ;
}

G::string_view G::string_view::substr( std::size_t pos , std::size_t count ) const
{
	if( (empty() && pos!=0U) || pos > m_n )
		throw std::out_of_range( "string_view::substr" ) ;
	else if( pos == m_n )
		return { m_p , std::size_t(0U) } ; // (more than the standard requires)
	else
		return { m_p + pos , std::min(m_n-pos,count) } ;
}

std::size_t G::string_view::find( char c , std::size_t pos ) const noexcept
{
	if( empty() || pos >= m_n ) return std::string::npos ;
	const char * p = m_p + pos ;
	std::size_t n = m_n - pos ;
	for( ; n ; p++ , n-- , pos++ )
	{
		if( *p == c )
			return pos ;
	}
	return std::string::npos ;
}

std::size_t G::string_view::find( const char * substr_p , std::size_t pos , std::size_t substr_n ) const
{
	return find( string_view(substr_p,substr_n) , pos ) ;
}

std::size_t G::string_view::find( string_view substr , std::size_t pos ) const
{
	if( empty() || pos >= m_n ) return std::string::npos ;
	if( substr.empty() ) return pos ;
	auto const end = m_p + m_n ;
	auto p = std::search( m_p+pos , end , substr.m_p , substr.m_p+substr.m_n ) ;
	return p == end ? std::string::npos : std::distance(m_p,p) ;
}

std::size_t G::string_view::find_first_of( const char * chars , std::size_t pos , std::size_t chars_size ) const noexcept
{
	return find_first_of( string_view(chars,chars_size) , pos ) ;
}

std::size_t G::string_view::find_first_of( string_view chars , std::size_t pos ) const noexcept
{
	if( empty() || pos >= m_n || chars.empty() ) return std::string::npos ;
	const char * p = m_p + pos ;
	std::size_t n = m_n - pos ;
	for( ; n ; p++ , n-- , pos++ )
	{
		const std::size_t i_end = chars.size() ;
		for( std::size_t i = 0U ; i < i_end ; i++ )
		{
			if( *p == chars[i] )
				return pos ;
		}
	}
	return std::string::npos ;
}

std::size_t G::string_view::find_first_not_of( char c , std::size_t pos ) const noexcept
{
	if( empty() || pos >= m_n ) return std::string::npos ;
	const char * p = m_p + pos ;
	std::size_t n = m_n - pos ;
	for( ; n ; p++ , n-- , pos++ )
	{
		if( *p != c )
			return pos ;
	}
	return std::string::npos ;
}

std::size_t G::string_view::find_first_not_of( const char * chars , std::size_t pos , std::size_t chars_size ) const noexcept
{
	return find_first_not_of( string_view(chars,chars_size) , pos ) ;
}

std::size_t G::string_view::find_first_not_of( string_view chars , std::size_t pos ) const noexcept
{
	if( empty() || pos >= m_n ) return std::string::npos ;
	const char * p = m_p + pos ;
	std::size_t n = m_n - pos ;
	for( ; n ; p++ , n-- , pos++ )
	{
		bool match = false ;
		const std::size_t i_end = chars.size() ;
		for( std::size_t i = 0U ; !match && i < i_end ; i++ )
		{
			if( *p == chars[i] )
				match = true ;
		}
		if( !match )
			return pos ;
	}
	return std::string::npos ;
}

std::size_t G::string_view::find_last_of( const char * chars , std::size_t pos , std::size_t chars_size ) const noexcept
{
	return find_last_of( string_view(chars,chars_size) , pos ) ;
}

std::size_t G::string_view::find_last_of( string_view chars , std::size_t pos ) const noexcept
{
	if( empty() ) return std::string::npos ;
	if( pos >= m_n ) pos = m_n - 1U ;
	const char * p = data() + pos ;
	std::size_t count = pos ;
	for( std::size_t n = 0U ; n <= count ; n++ , p-- , pos-- )
	{
		bool match = false ;
		const std::size_t i_end = chars.size() ;
		for( std::size_t i = 0U ; !match && i < i_end ; i++ )
		{
			if( *p == chars[i] )
				match = true ;
		}
		if( match )
			return pos ;
	}
	return std::string::npos ;
}

std::size_t G::string_view::find_last_not_of( const char * chars , std::size_t pos , std::size_t chars_size ) const noexcept
{
	return find_last_not_of( string_view(chars,chars_size) , pos ) ;
}

std::size_t G::string_view::find_last_not_of( string_view chars , std::size_t pos ) const noexcept
{
	if( empty() ) return std::string::npos ;
	if( pos >= m_n ) pos = m_n - 1U ;
	const char * p = data() + pos ;
	std::size_t count = pos ;
	for( std::size_t n = 0U ; n <= count ; n++ , p-- , pos-- )
	{
		bool match = false ;
		const std::size_t i_end = chars.size() ;
		for( std::size_t i = 0U ; !match && i < i_end ; i++ )
		{
			if( *p == chars[i] )
				match = true ;
		}
		if( !match )
			return pos ;
	}
	return std::string::npos ;
}

std::size_t G::string_view::rfind( char c , std::size_t pos ) const noexcept
{
	if( empty() || pos == 0U ) return std::string::npos ;
	if( pos == std::string::npos || pos > m_n ) pos = m_n ;
	const char * p = m_p + pos - 1U ;
	std::size_t n = pos - 1U ;
	for( ; n ; n-- , p-- )
	{
		if( *p == c )
			return n ;
	}
	return std::string::npos ;
}

#endif
