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
//
// gbase64.cpp
//

#include "gdef.h"
#include "gbase64.h"
#include "gassert.h"
#include "gstr.h"
#include <cstring>

namespace imp
{
	const char * character_map = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" ;
	char pad = '=' ;

	std::string encode( const std::string & s , const std::string & eol ) ;
	std::string decode( const std::string & s , bool & error ) ;
	bool valid( const std::string & ) ;

	g_uint32_t numeric( char c ) ;
	void accumulate_8( g_uint32_t & n , std::string::const_iterator & , std::string::const_iterator , int & ) ;
	size_t hi_6( g_uint32_t n ) ;
	void generate_6( g_uint32_t & n , int & i , std::string & result ) ;
	char to_char( g_uint32_t n ) ;
	size_t index( char c , bool & error ) ;
	void accumulate_6( g_uint32_t & n , const std::string & , size_t & , size_t & , bool & error ) ;
	g_uint32_t hi_8( g_uint32_t n ) ;
	void generate_8( g_uint32_t & n , size_t & i , std::string & result , bool & error ) ;
}

std::string G::Base64::encode( const std::string & s_in )
{
	return encode( s_in , "\015\012" ) ;
}

std::string G::Base64::encode( const std::string & s_in , const std::string & eol )
{
	return imp::encode( s_in , eol ) ;
}

std::string G::Base64::decode( const std::string & s , bool strict )
{
	bool error = false ;
	std::string result = imp::decode( s , error ) ;
	if( strict && error )
		throw Error() ;
	return result ;
}

bool G::Base64::valid( const std::string & s )
{
	return imp::valid( s ) ;
}

// ==

g_uint32_t imp::numeric( char c )
{
	return static_cast<g_uint32_t>( static_cast<unsigned char>(c) ) ;
}

void imp::accumulate_8( g_uint32_t & n , std::string::const_iterator & p ,
	std::string::const_iterator end , int & i )
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

size_t imp::hi_6( g_uint32_t n )
{
	return (n >> 18U) & 0x3F ;
}

void imp::generate_6( g_uint32_t & n , int & i , std::string & result )
{
	char c = i-- >= 0 ? character_map[hi_6(n)] : pad ;
	result.append( 1U , c ) ;
	n <<= 6U ;
}

std::string imp::encode( const std::string & s_in , const std::string & eol )
{
	std::string result ;
	size_t blocks = 0U ;
	for( std::string::const_iterator p = s_in.begin() ; p != s_in.end() ; blocks++ )
	{
		if( blocks && (blocks % 19U) == 0U )
			result.append( eol ) ;

		g_uint32_t n = 0UL ;
		int i = 0 ;
		accumulate_8( n , p , s_in.end() , i ) ;
		accumulate_8( n , p , s_in.end() , i ) ;
		accumulate_8( n , p , s_in.end() , i ) ;
		generate_6( n , i , result ) ;
		generate_6( n , i , result ) ;
		generate_6( n , i , result ) ;
		generate_6( n , i , result ) ;
	}

	return result ;
}

char imp::to_char( g_uint32_t n )
{
	return static_cast<char>(static_cast<unsigned char>(n)) ;
}

size_t imp::index( char c , bool & error )
{
	const char * p = std::strchr( character_map , c ) ;
	error = error || !c || !p ;
	if( p == nullptr )
		return 0U ;
	else
		return static_cast<size_t>( p - character_map ) ;
}

void imp::accumulate_6( g_uint32_t & n , const std::string & s , size_t & i , size_t & bits , bool & error )
{
	n <<= 6U ;
	if( i == s.length() )
	{
	}
	else if( s.at(i) == pad )
	{
		i++ ;
	}
	else
	{
		n |= index( s.at(i++) , error ) ;
		bits += 6U ;
	}
}

g_uint32_t imp::hi_8( g_uint32_t n )
{
	return (n >> 16U) & 0xff ;
}

void imp::generate_8( g_uint32_t & n , size_t & bits , std::string & result , bool & error )
{
	if( bits >= 8U )
	{
		bits -= 8U ;
		result.append( 1U , to_char(hi_8(n)) ) ;
		n <<= 8U ;
	}
	else if( hi_8(n) )
	{
		error = true ;
	}
}

std::string imp::decode( const std::string & s , bool & error )
{
	std::string result ;
	size_t i = 0U ;
	for( const char * p = s.c_str() ; p[i] ; )
	{
		if( *p == '\r' || *p == '\n' )
		{
			p++ ;
			continue ;
		}

		// four input characters encode 4*6 bits, so three output bytes
		g_uint32_t n = 0UL ; // up to 24 bits
		size_t bits = 0U ;
		accumulate_6( n , s , i , bits , error ) ;
		accumulate_6( n , s , i , bits , error ) ;
		accumulate_6( n , s , i , bits , error ) ;
		accumulate_6( n , s , i , bits , error ) ;
		if( bits < 8U ) error = true ; // 6 bits cannot make a byte
		generate_8( n , bits , result , error ) ;
		generate_8( n , bits , result , error ) ;
		generate_8( n , bits , result , error ) ;
	}
	return result ;
}

bool imp::valid( const std::string & s )
{
	bool error = false ;
	std::string result = decode( s , error ) ;
	G_IGNORE_VARIABLE(result) ;
	return !error ;
}

/// \file gbase64.cpp
