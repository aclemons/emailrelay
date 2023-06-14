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
/// \file gbase64.cpp
///

#include "gdef.h"
#include "gbase64.h"
#include "gstringview.h"
#include "gstr.h"
#include <algorithm>
#include <iterator>

namespace G
{
	namespace Base64Imp
	{
		#ifdef G_WINDOWS
		using uint32_type = volatile g_uint32_t ; // volatile as workround for compiler bug: MSVC 2019 16.6.2 /02 /Ob2
		#else
		using uint32_type = g_uint32_t ;
		#endif

		static constexpr string_view character_map_with_pad = "=ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"_sv ;
		static constexpr string_view character_map( character_map_with_pad.data()+1 , character_map_with_pad.size()-1 ) ;
		static constexpr char pad = '=' ;

		static_assert( character_map_with_pad.size() == 1+26+26+10+2 , "" ) ;
		static_assert( character_map.size() == 26+26+10+2 , "" ) ;

		using iterator_in = string_view::const_iterator ;
		using iterator_out = std::back_insert_iterator<std::string> ;

		std::string encode( string_view , string_view eol ) ;
		std::string decode( string_view , bool do_throw , bool strict ) ;
		bool valid( string_view , bool strict ) ;

		void encode_imp( iterator_out , string_view , string_view , std::size_t ) ;
		void decode_imp( iterator_out , string_view s , bool & error ) ;
		void generate_6( uint32_type & n , int & i , iterator_out & ) ;
		void accumulate_8( uint32_type & n , iterator_in & , iterator_in , int & ) ;
		void accumulate_6( g_uint32_t & n , iterator_in & , iterator_in , std::size_t & , bool & error ) ;
		void generate_8( g_uint32_t & n , std::size_t & i , iterator_out & , bool & error ) ;
		std::size_t index( char c , bool & error ) noexcept ;
		bool strictlyValid( string_view ) noexcept ;

		constexpr char to_char( g_uint32_t n ) noexcept
		{
			return static_cast<char>( static_cast<unsigned char>(n) ) ;
		}
		constexpr g_uint32_t numeric( char c ) noexcept
		{
			return static_cast<g_uint32_t>( static_cast<unsigned char>(c) ) ;
		}
		constexpr std::size_t hi_6( g_uint32_t n ) noexcept
		{
			return (n >> 18U) & 0x3FU ;
		}
		constexpr g_uint32_t hi_8( g_uint32_t n ) noexcept
		{
			return (n >> 16U) & 0xFFU ;
		}
	}
}

// ==

std::string G::Base64::encode( string_view s , string_view eol )
{
	return Base64Imp::encode( s , eol ) ;
}

std::string G::Base64::decode( string_view s , bool do_throw , bool strict )
{
	return Base64Imp::decode( s , do_throw , strict ) ;
}

bool G::Base64::valid( string_view s , bool strict )
{
	return Base64Imp::valid( s , strict ) ;
}

// ==

std::string G::Base64Imp::encode( string_view input , string_view eol )
{
	std::string result ;
	result.reserve( input.size() + input.size()/2U ) ;
	encode_imp( std::back_inserter(result) , input , eol , 19U ) ;
	return result ;
}

void G::Base64Imp::encode_imp( iterator_out result_p , string_view input , string_view eol , std::size_t blocks_per_line )
{
	std::size_t blocks = 0U ;
	auto const end = input.end() ;
	for( auto p = input.begin() ; p != end ; blocks++ )
	{
		if( !eol.empty() && blocks != 0U && (blocks % blocks_per_line) == 0U )
			std::copy( eol.begin() , eol.end() , result_p ) ;

		uint32_type n = 0UL ;
		int i = 0 ;
		accumulate_8( n , p , end , i ) ;
		accumulate_8( n , p , end , i ) ;
		accumulate_8( n , p , end , i ) ;
		generate_6( n , i , result_p ) ;
		generate_6( n , i , result_p ) ;
		generate_6( n , i , result_p ) ;
		generate_6( n , i , result_p ) ;
	}
}

std::string G::Base64Imp::decode( string_view input , bool do_throw , bool strict )
{
	bool error = false ;
	if( strict && !strictlyValid(input) )
		error = true ;

	std::string result ;
	result.reserve( input.size() ) ;
	decode_imp( std::back_inserter(result) , input , error ) ;

	if( error )
		result.clear() ;
	if( error && do_throw )
		throw Base64::Error() ;

	return result ;
}

void G::Base64Imp::decode_imp( iterator_out result_p , string_view s , bool & error )
{
	auto const end = s.end() ;
	for( auto p = s.begin() ; p != end ; )
	{
		if( *p == '\r' || *p == '\n' || *p == ' ' )
		{
			++p ;
			continue ;
		}

		// four input characters encode 4*6 bits, so three output bytes
		g_uint32_t n = 0UL ; // up to 24 bits
		std::size_t bits = 0U ;
		accumulate_6( n , p , end , bits , error ) ;
		accumulate_6( n , p , end , bits , error ) ;
		accumulate_6( n , p , end , bits , error ) ;
		accumulate_6( n , p , end , bits , error ) ;
		if( bits < 8U ) error = true ; // 6 bits cannot make a byte
		generate_8( n , bits , result_p , error ) ;
		generate_8( n , bits , result_p , error ) ;
		generate_8( n , bits , result_p , error ) ;
	}
}

bool G::Base64Imp::valid( string_view input , bool strict )
{
	if( strict && !strictlyValid(input) )
		return false ;

	bool error = false ;
	std::string result ;
	result.reserve( input.size() ) ;
	decode_imp( std::back_inserter(result) , input , error ) ;
	return !error ;
}

bool G::Base64Imp::strictlyValid( string_view s ) noexcept
{
	if( s.empty() )
		return true ;

	if( s.size() == 1 )
		return false ; // 6 bits cannot make a byte

	if( std::string::npos == s.find_first_not_of(character_map) )
		return true ;

	if( std::string::npos != s.find_first_not_of(character_map_with_pad) )
		return false ;

	std::size_t pos = s.find( pad ) ;
	if( (pos+1U) == s.size() && s[pos] == pad && (s.size()&3U) == 0U )
		return true ;

	if( (pos+2U) == s.size() && s[pos] == pad && s[pos+1U] == pad && (s.size()&3U) == 0U )
		return true ;

	return false ;
}

void G::Base64Imp::accumulate_8( uint32_type & n , iterator_in & p , iterator_in end , int & i )
{
	char c = p == end ? '\0' : *p ;
	n <<= 8U ;
	n |= numeric(c) ;
	if( p != end )
	{
		++p ;
		++i ;
	}
}

void G::Base64Imp::generate_6( uint32_type & n , int & i , iterator_out & result )
{
	size_t index = hi_6( n ) ;
	char c = i-- >= 0 ? character_map[index] : pad ;
	*result++ = c ;
	n <<= 6U ;
}

void G::Base64Imp::accumulate_6( g_uint32_t & n , iterator_in & p , iterator_in end ,
	std::size_t & bits , bool & error )
{
	n <<= 6U ;
	if( p == end )
	{
	}
	else if( *p == pad )
	{
		++p ;
	}
	else
	{
		n |= index( *p++ , error ) ;
		bits += 6U ;
	}
}

void G::Base64Imp::generate_8( g_uint32_t & n , std::size_t & bits , iterator_out & result , bool & error )
{
	if( bits >= 8U )
	{
		bits -= 8U ;
		*result++ = to_char(hi_8(n)) ;
		n <<= 8U ;
	}
	else if( hi_8(n) != 0U )
	{
		error = true ;
	}
}

std::size_t G::Base64Imp::index( char c , bool & error ) noexcept
{
	std::size_t pos = character_map.find( c ) ;
	error = error || (c=='\0') || pos == std::string::npos ;
	return pos == std::string::npos ? std::size_t(0) : pos ;
}

