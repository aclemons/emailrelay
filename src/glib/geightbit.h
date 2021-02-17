//
// Copyright (C) 2001-2021 Graeme Walker <graeme_walker@users.sourceforge.net>
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
	namespace EightBitImp /// An implementation namespace for G::eightbit().
	{
		template <typename T, T t, unsigned int N>
		struct extend /// Evaluates a type 'T' bitmask comprising N copies of byte 't'.
		{
			// eg. 0x80808080
			static constexpr T value = ( extend<T,t,N-1U>::value << 8U ) | t ;
		} ;

		template <typename T, T t>
		struct extend<T,t,1U> /// Terminal specialisation of extend<>.
		{
			static constexpr T value = t ;
		} ;

		template <typename T>
		struct is8bit_fn /// Functor returning true if 't' AND-ed with an extend mask based on 0x80 is non-zero.
		{
			inline constexpr bool operator()( T t ) const noexcept { return ( t & extend<T,0x80,sizeof(T)>::value ) != 0 ; }
		} ;

		template <typename T>
		inline
		bool is8bit_imp_int( const unsigned char * p , std::size_t n )
		{
			is8bit_fn<T> fn ;
			std::size_t j = 0U ;
			for( std::size_t i = 0U ; i < n ; i++ , j += sizeof(T) )
			{
				T t = 0 ;
				std::memcpy( &t , &p[j] , sizeof(T) ) ; // strict-aliasing shenanigans, gets optimised out
				if( fn(t) )
					return true ;
			}
			return false ;
		}

		inline bool is8bit_imp_uchar( const unsigned char * p0 , std::size_t n )
		{
			const unsigned char * end = p0 + n ;
			return std::find_if( p0 , end , [](char c){return (c & 0x80U)!=0U ;} ) != end ;
		}

		inline bool is8bit_slow( const unsigned char * p0 , std::size_t n )
		{
			return is8bit_imp_uchar( p0 , n ) ;
		}

		inline bool is8bit_faster( const unsigned char * p0 , std::size_t n )
		{
			using byte_t = unsigned char ;
			using int_t = unsigned long ; // or uint if faster
			const void * vp1 = G::align<int_t>( p0 , n ) ;
			if( vp1 == nullptr )
			{
				return is8bit_slow( p0 , n ) ;
			}
			else
			{
				// split into three ranges, with a big aligned range in the middle
				const byte_t * p1 = static_cast<const byte_t*>(vp1) ;
				const std::size_t n0 = std::distance( p0 , p1 ) ;
				const std::size_t n1 = G::align_mask<int_t>( n - n0 ) ;
				const std::size_t ni1 = G::align_shift<int_t>( n - n0 ) ;
				const std::size_t n2 = n - n0 - n1 ;
				const byte_t * p2 = p0 + n0 + n1 ;
				return
					is8bit_imp_uchar(p0,n0) ||
					is8bit_imp_int<int_t>(p1,ni1) ||
					is8bit_imp_uchar(p2,n2) ;
			}
		}
	}

	/// Returns true if the given data buffer contains a byte greater than 127.
	/// An overload for an unsigned char buffer.
	///
	inline bool eightbit( const unsigned char * p , std::size_t n )
	{
		namespace imp = EightBitImp ;
		return imp::is8bit_faster( p , n ) ;
	}

	/// Returns true if the given data buffer contains a character greater than 127.
	/// An overload for a char buffer.
	///
	inline bool eightbit( const char * p , std::size_t n )
	{
		namespace imp = EightBitImp ;
		return imp::is8bit_faster( reinterpret_cast<const unsigned char *>(p) , n ) ;
	}

	/// Returns true if the given data buffer contains a byte greater than 127.
	/// An overload for an unsigned char buffer, with no optimisation.
	///
	inline bool eightbit( const unsigned char * p , std::size_t n , int )
	{
		namespace imp = EightBitImp ;
		return imp::is8bit_slow( p , n ) ;
	}

	/// Returns true if the given data buffer contains a character greater than 127.
	/// An overload for a char buffer, with no optimisation.
	///
	inline bool eightbit( const char * p , std::size_t n , int )
	{
		namespace imp = EightBitImp ;
		return imp::is8bit_slow( reinterpret_cast<const unsigned char *>(p) , n ) ;
	}
}

#endif
