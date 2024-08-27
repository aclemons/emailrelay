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
/// \file ghashstate.h
///

#ifndef G_HASH_STATE_H
#define G_HASH_STATE_H

#include "gdef.h"
#include <string>
#include <array>

namespace G
{
	class HashStateImp ;
	template <unsigned int N, typename U, typename S> class HashState ;
}

//| \class G::HashStateImp
/// The non-template part of G::HashState.
///
class G::HashStateImp
{
public:
	template <typename U> static std::string extension( U n ) ;
		///< Returns the given data size as a four-character
		///< string.

protected:
	template <typename U> static void convert_( U n , std::string::iterator p ) ;
		///< Encodes the given value into four characters.

public:
	HashStateImp() = delete ;
} ;

//| \class G::HashState
/// Functions for representing the intermediate state of a hash function
/// as a non-printable string. The input is an array of 'N/4' 32-bit
/// values. The output is a string of N non-printable characters, or N+4
/// characters if also including the data size. The 'U' type can be more
/// than 32 bits wide but it should hold values of no more than 32 bits
/// significance.
/// \see G::Hash::printable
///
template <unsigned int N, typename U, typename S>
class G::HashState : public HashStateImp
{
public:
	using uint_type = U ;
	using size_type = S ;
	static_assert( N != 0 && (N%4) == 0 , "hash state size must be a multiple of four" ) ;

	static std::string encode( const uint_type * ) ;
		///< Returns the hash state as an N-character string of
		///< non-printing characters.

	static std::string encode( const uint_type * , size_type n ) ;
		///< Returns the hash state as a string that also
		///< has the original data size as a four-character
		///< extension().

	static std::string encode( uint_type hi , uint_type low , const uint_type * ) ;
		///< An overload with a hi/low bit count.

	static std::string encode( uint_type hi , uint_type low , uint_type v0 , uint_type v1 ,
		uint_type v2 , uint_type v3 , uint_type v4 = 0 ) ;
			///< An overload for N=16 or N=20 with broken-out
			///< values and a hi/low bit count.

	static void decode( const std::string & s , uint_type * values_out , size_type & size_out ) ;
		///< Converts an encode()d string back into a hash
		///< state of N/4 integers and a data size returned
		///< by reference. The data size is returned as zero
		///< if the input string is only N characters long.

	static void decode( const std::string & , uint_type & size_hi_out , uint_type & size_low_out ,
		uint_type * value_0 , uint_type * value_1 , uint_type * value_2 , uint_type * value_3 ,
		uint_type * value_4 = nullptr ) ;
			///< An overload for N=16 or N=20 with broken-out
			///< values and hi/low bit count.

	static void decode( const std::string & , uint_type & size_hi_out , uint_type & size_low_out ,
		uint_type * values_out ) ;
			///< An overload for a hi/low bit count.

public:
	HashState() = delete ;

private:
	static void convert( char hi , char himid , char lomid , char lo , uint_type & n ) ;
	static void convert( const std::string & str , uint_type & n ) ;
	static void convert( const std::string & s , uint_type * state ) ;
	static void convert( const std::string & s , uint_type * state , size_type & ) ;
} ;

template <unsigned int N, typename U, typename S>
std::string G::HashState<N,U,S>::encode( const uint_type * values )
{
	std::string result( N , '\0' ) ;
	for( std::size_t i = 0U ; i < N/4 ; i++ )
	{
		convert_( values[i] , result.begin() + (i*4U) ) ; // NOLINT narrowing
	}
	return result ;
}

template <unsigned int N, typename U, typename S>
std::string G::HashState<N,U,S>::encode( const uint_type * values , size_type n )
{
	std::string result( N+4U , '\0' ) ;
	for( std::size_t i = 0U ; i < N/4 ; i++ )
	{
		convert_( values[i] , result.begin() + (i*4U) ) ; // NOLINT narrowing
	}
	convert_( n , result.begin() + N ) ;
	return result ;
}

template <unsigned int N, typename U, typename S>
std::string G::HashState<N,U,S>::encode( uint_type hi , uint_type low , const uint_type * values )
{
	uint_type n = hi ;
	n <<= 29 ;
	n |= ( low >> 3 ) ;
	return encode( values , n ) ;
}

template <unsigned int N, typename U, typename S>
std::string G::HashState<N,U,S>::encode( uint_type hi , uint_type low , uint_type v0 ,
	uint_type v1 , uint_type v2 , uint_type v3 , uint_type v4 )
{
	unsigned int NN = N ; // workround for msvc C4127
	uint_type n = hi ;
	n <<= 29 ;
	n |= ( low >> 3 ) ;
	std::array<uint_type,N/4> values {} ;
	if( NN > 0 ) values[0] = v0 ;
	if( NN > 4 ) values[1] = v1 ;
	if( NN > 8 ) values[2] = v2 ;
	if( NN > 12 ) values[3] = v3 ;
	if( NN > 16 ) values[4] = v4 ;
	return encode( values.data() , n ) ;
}

template <typename U>
std::string G::HashStateImp::extension( U n )
{
	std::string result( 4U , '\0' ) ;
	convert_( n , result.begin() ) ;
	return result ;
}

template <unsigned int N, typename U, typename S>
void G::HashState<N,U,S>::decode( const std::string & str , uint_type * values_out , size_type & size_out )
{
	if( str.length() < (N+4U) )
		return decode( str+std::string(N+4U,'\0') , values_out , size_out ) ; // call ourselves again if too short // NOLINT recursion
	convert( str , values_out , size_out ) ;
}

template <unsigned int N, typename U, typename S>
void G::HashState<N,U,S>::decode( const std::string & str , uint_type & hi , uint_type & low ,
	uint_type * v0 , uint_type * v1 , uint_type * v2 , uint_type * v3 , uint_type * v4 )
{
	if( str.length() < (N+4U) )
		return decode( str+std::string(N+4U,'\0') , hi , low , v0 , v1 , v2 , v3 , v4 ) ; // call ourselves again if too short // NOLINT recursion
	std::array<uint_type,N/4> values {} ;
	uint_type n ;
	convert( str , values.data() , n ) ;
	if( v0 && N > 0 ) *v0 = values[0] ;
	if( v1 && N > 4 ) *v1 = values[1] ;
	if( v2 && N > 8 ) *v2 = values[2] ;
	if( v3 && N > 12 ) *v3 = values[3] ;
	if( v4 && N > 16 ) *v4 = values[4] ;
	hi = ( n >> 29 ) ;
	low = ( n << 3 ) & 0xffffffffUL ;
}

template <unsigned int N, typename U, typename S>
void G::HashState<N,U,S>::decode( const std::string & str , uint_type & hi , uint_type & low ,
	uint_type * values_out )
{
	if( str.length() < (N+4U) )
		return decode( str+std::string(N+4U,'\0') , hi , low , values_out ) ; // call ourselves again if too short // NOLINT recursion
	uint_type n ;
	convert( str , values_out , n ) ;
	hi = ( n >> 29 ) ;
	low = ( n << 3 ) & 0xffffffffUL ;
}

template <typename U>
void G::HashStateImp::convert_( U n , std::string::iterator p_out )
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
	for( std::size_t i = 0U ; i < (N/4U) ; i++ )
	{
		convert( str.at(i*4U+3U) , str.at(i*4U+2U) , str.at(i*4U+1U) , str.at(i*4U+0U) , state_out[i] ) ;
	}
}

template <unsigned int N, typename U, typename S>
void G::HashState<N,U,S>::convert( const std::string & str , uint_type * state_out , size_type & n_out )
{
	convert( str , state_out ) ;
	uint_type nn = 0 ;
	convert( str.at(N+3U) , str.at(N+2U) , str.at(N+1U) , str.at(N) , nn ) ;
	n_out = static_cast<size_type>(nn) ;
}

#endif
