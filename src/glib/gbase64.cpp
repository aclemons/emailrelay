//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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

namespace
{
	const char * character_map = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" ;
	char pad = '=' ;
}

g_uint32_t G::Base64::numeric( char c )
{
	return static_cast<g_uint32_t>( static_cast<unsigned char>(c) ) ;
}

void G::Base64::accumulate_8( g_uint32_t & n , std::string::const_iterator & p ,
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

size_t G::Base64::hi_6( g_uint32_t n )
{
	return (n >> 18U) & 0x3F ;
}

void G::Base64::generate_6( g_uint32_t & n , int & i , std::string & result )
{
	char c = i-- >= 0 ? character_map[hi_6(n)] : pad ;
	result.append( 1U , c ) ;
	n <<= 6U ;
}

std::string G::Base64::encode( const std::string & s_in )
{
	return encode( s_in , "\015\012" ) ;
}

std::string G::Base64::encode( const std::string & s_in , const std::string & eol )
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

// ---

char G::Base64::to_char( g_uint32_t n )
{
	return static_cast<char>(static_cast<unsigned char>(n)) ;
}

size_t G::Base64::index( char c , bool & error )
{
	const char * p = std::strchr( character_map , c ) ;
	error = error || !c || !p ;
	if( p == NULL ) 
		return 0U ;
	else
		return static_cast<size_t>( p - character_map ) ;
}

size_t G::Base64::accumulate_6( g_uint32_t & n , char c_in , int & n_out , bool & error )
{
	n <<= 6U ;
	if( c_in != pad ) 
	{
		n |= index(c_in,error) ;
		n_out++ ;
	}
	return c_in != '\0' ;
}

g_uint32_t G::Base64::hi_8( g_uint32_t n )
{
	return (n >> 16U) & 0xff ;
}

void G::Base64::generate_8( g_uint32_t & n , int & n_out , std::string & result )
{
	if( n_out-- > 0 )
		result.append( 1U , to_char(hi_8(n)) ) ;
	n <<= 8U ;
}

std::string G::Base64::decode( const std::string & s )
{
	bool error = false ;
	std::string result = decode( s , error ) ;
	if( error )
		throw Error() ;
	return result ;
}

std::string G::Base64::decode( const std::string & s , bool & error )
{
	std::string result ;
	for( const char * p = s.c_str() ; *p ; )
	{
		if( *p == '\r' || *p == '\n' ) 
		{ 
			p++ ; 
			continue ; 
		}

		g_uint32_t n = 0UL ;
		size_t i = 0U ;
		int n_out = -1 ;
		i += accumulate_6( n , p[i] , n_out , error ) ;
		i += accumulate_6( n , p[i] , n_out , error ) ;
		i += accumulate_6( n , p[i] , n_out , error ) ;
		i += accumulate_6( n , p[i] , n_out , error ) ;
		p += i ;
		generate_8( n , n_out , result ) ;
		generate_8( n , n_out , result ) ;
		generate_8( n , n_out , result ) ;
	}
	return result ;
}

bool G::Base64::valid( const std::string & s )
{
	bool error = false ;
	G_IGNORE_RETURN(std::string) decode( s , error ) ;
	return !error ;
}

/// \file gbase64.cpp
