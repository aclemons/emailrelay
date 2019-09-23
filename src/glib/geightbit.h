//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file geightbit.h
///

#ifndef G_EIGHTBIT_H
#define G_EIGHTBIT_H

#include "gdef.h"
#include "galign.h"
#include <memory>
#include <algorithm>

namespace G
{
	/// \namespace G::EightBitImp
	/// An implementation namespace for G::eightbit().
	///
	namespace EightBitImp
	{
		template <typename T, T t, unsigned int N>
		struct extend /// Evaluates a type 'T' bitmask comprising N copies of byte 't'.
		{
			// eg. 0x80808080
			static g__constexpr T value = ( extend<T,t,N-1U>::value << 8U ) | t ;
		} ;

		template <typename T, T t>
		struct extend<T,t,1U> /// Terminal specialisation of extend<>.
		{
			static g__constexpr T value = t ;
		} ;

		template <typename T>
		struct tester /// Functor returning true if 't' AND-ed with an extend mask based on 0x80 is non-zero.
		{
			inline g__constexpr_fn bool operator()( T t ) const g__noexcept { return ( t & extend<T,0x80,sizeof(T)>::value ) != 0 ; }
		} ;

		template <typename T>
		inline
		bool test_imp( const T * p , size_t n )
		{
			const T * const end = p + n ;
			return std::find_if( p , end , tester<T>() ) != end ;
		}

		inline bool test( const unsigned char * p0 , size_t n , int /*slow*/ )
		{
			return test_imp( p0 , n ) ;
		}

		inline bool test( const unsigned char * p0 , size_t n )
		{
			typedef unsigned long big_t ; // or uint if faster
			const void * vp1 = G::align<big_t>( p0 , n ) ;
			if( vp1 == nullptr )
			{
				return test_imp( p0 , n ) ;
			}
			else
			{
				// split into three ranges, with a big aligned range in the middle
				typedef unsigned char byte_t ;
				const big_t * pi1 = static_cast<const big_t*>(vp1) ;
				const byte_t * p1 = static_cast<const byte_t*>(vp1) ;
				const size_t n0 = std::distance( p0 , p1 ) ;
				const size_t n1 = G::AlignImp::mask<big_t>( n - n0 ) ;
				const size_t ni1 = G::AlignImp::shift<big_t>( n - n0 ) ;
				const size_t n2 = n - n0 - n1 ;
				const byte_t * p2 = p0 + n0 + n1 ;
				return
					test_imp(p0,n0) ||
					test_imp(pi1,ni1) ||
					test_imp(p2,n2) ;
			}
		}
	}

/// Returns true if the given unsigned-char data buffer contains a byte greater than 127.
///
inline bool eightbit( const unsigned char * p , size_t n )
{
	namespace imp = EightBitImp ;
	return imp::test( p , n ) ;
}

/// An overload with no alignment optimisation.
///
inline bool eightbit( const unsigned char * p , size_t n , int slow )
{
	namespace imp = EightBitImp ;
	return imp::test( p , n , slow ) ;
}

/// Returns true if the given data buffer contains a character with its top bit set.
///
inline bool eightbit( const char * p , size_t n )
{
	namespace imp = EightBitImp ;
	return imp::test( reinterpret_cast<const unsigned char *>(p) , n ) ;
}

/// An overload with no alignment optimisation.
///
inline bool eightbit( const char * p , size_t n , int slow )
{
	namespace imp = EightBitImp ;
	return imp::test( reinterpret_cast<const unsigned char *>(p) , n , slow ) ;
}

}

#endif
