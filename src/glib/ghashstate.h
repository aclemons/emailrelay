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
///
/// \file ghashstate.h
///

#ifndef G_HASH_STATE__H
#define G_HASH_STATE__H

#include "gdef.h"
#include <string>

namespace G
{
	template <unsigned int N, typename U, typename S> class HashState ;
}

/// \class G::HashState
/// Helper functions for representing the state of a hash function as
/// a non-printable string. The hash state must be an array of 'N/4'
/// 32-bit values, with encoded strings being N or N+4 characters.
/// \see G::Hash::printable
///
template <unsigned int N, typename U, typename S>
class G::HashState
{
public:
	typedef U uint_type ;
	typedef S size_type ;

	static std::string encode( const uint_type * ) ;
		///< Returns the hash state as a string, typically
		///< containing non-printing characters.

	static std::string encode( const uint_type * , size_type n ) ;
		///< Returns the hash state as a string that also
		///< encodes the data size.

	static void decode( const std::string & , uint_type * , size_type & ) ;
		///< Converts an encode()d string back into a hash
		///< state of N integers and a data size returned
		///< by reference.

private:
	HashState() ;
	static void convert( uint_type n , char * p ) ;
	static void convert( char hi , char himid , char lomid , char lo , uint_type & n ) ;
	static void convert( const std::string & str , uint_type & n ) ;
	static void convert( const std::string & s , uint_type * state ) ;
	static void convert( const std::string & s , uint_type * state , size_type & ) ;
} ;

template <unsigned int N, typename U, typename S>
std::string G::HashState<N,U,S>::encode( const uint_type * state )
{
	std::string result( N , '\0' ) ;
	for( size_t i = 0U ; i < N/4 ; i++ )
	{
		convert( state[i] , &result[i*4U] ) ;
	}
	return result ;
}

template <unsigned int N, typename U, typename S>
std::string G::HashState<N,U,S>::encode( const uint_type * state , size_type n )
{
	std::string result( N+4U , '\0' ) ;
	for( size_t i = 0U ; i < N/4 ; i++ )
	{
		convert( state[i] , &result[i*4U] ) ;
	}
	convert( n , &result[N] ) ;
	return result ;
}

template <unsigned int N, typename U, typename S>
void G::HashState<N,U,S>::decode( const std::string & str , uint_type * state_out , size_type & n_out )
{
	if( str.length() < (N+4U) )
		return decode( str+std::string(N+4U,'\0') , state_out , n_out ) ; // resurse if too short
	convert( str , state_out , n_out ) ;
}

template <unsigned int N, typename U, typename S>
void G::HashState<N,U,S>::convert( uint_type n , char * p_out )
{
	*p_out++ = static_cast<char>(n&0xffU) ; n >>= 8U ;
	*p_out++ = static_cast<char>(n&0xffU) ; n >>= 8U ;
	*p_out++ = static_cast<char>(n&0xffU) ; n >>= 8U ;
	*p_out++ = static_cast<char>(n&0xffU) ;
}

template <unsigned int N, typename U, typename S>
void G::HashState<N,U,S>::convert( char hi , char himid , char lomid , char lo , uint_type & n_out )
{
	n_out = 0U ;
	n_out |= static_cast<unsigned char>(hi) ; n_out <<= 8 ;
	n_out |= static_cast<unsigned char>(himid) ; n_out <<= 8 ;
	n_out |= static_cast<unsigned char>(lomid) ; n_out <<= 8 ;
	n_out |= static_cast<unsigned char>(lo) ;
}

template <unsigned int N, typename U, typename S>
void G::HashState<N,U,S>::convert( const std::string & str , uint_type & n_out )
{
	convert( str.at(3) , str.at(2) , str.at(1) , str.at(0) , n_out ) ;
}

template <unsigned int N, typename U, typename S>
void G::HashState<N,U,S>::convert( const std::string & str , uint_type * state_out )
{
	for( unsigned int i = 0U ; i < (N/4U) ; i++ )
	{
		convert( str.at(i*4U+3U) , str.at(i*4U+2U) , str.at(i*4U+1U) , str.at(i*4U+0U) , state_out[i] ) ;
	}
}

template <unsigned int N, typename U, typename S>
void G::HashState<N,U,S>::convert( const std::string & str , uint_type * state_out , size_type & n_out )
{
	convert( str , state_out ) ;
	uint_type nn ;
	convert( str.at(N+3U) , str.at(N+2U) , str.at(N+1U) , str.at(N) , nn ) ;
	n_out = static_cast<size_type>(nn) ;
}

#endif
