//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file galign.h
///

#ifndef G_ALIGN_H
#define G_ALIGN_H

#include "gdef.h"
#include <memory>

namespace G
{
	namespace AlignImp /// An implementation namespace for G::align().
	{
		template <unsigned int N>
		struct log2of /// Evaluates the number of bits in the template parameter N.
		{
			static constexpr unsigned int value = 1U + log2of<N/2>::value ;
		} ;

		template <>
		struct log2of<1U> /// Terminal specialisation of log2of<N>.
		{
			static constexpr unsigned int value = 0U ;
		} ;

		template <typename Talign, typename Tvalue>
		inline
		constexpr Tvalue mask( Tvalue n )
		{
			return n & (~(Tvalue(0))<<log2of<sizeof(Talign)>::value) ;
		}

		template <typename Talign, typename Tvalue>
		inline
		constexpr Tvalue shift( Tvalue n )
		{
			return n >> log2of<sizeof(Talign)>::value ;
		}

		template <typename Talign, typename Tchar>
		inline void * align_imp( const Tchar * p , const std::size_t n_in )
		{
			void * vp = const_cast<Tchar*>(p) ;
			std::size_t n = n_in ;
			return std::align( alignof(Talign) , sizeof(Talign) , vp , n ) ;
		}
	}

	/// \code
	/// Returns a pointer inside the given buffer that is aligned for
	/// values of type T.
	///
	/// \endcode
	template <typename T>
	inline void * align( const char * buffer , std::size_t buffer_size )
	{
		return AlignImp::align_imp<T>( buffer , buffer_size ) ;
	}

	/// \code
	/// Returns a pointer inside the given unsigned-char buffer that
	/// is aligned for values of type T.
	///
	/// \endcode
	template <typename T>
	inline void * align( const unsigned char * buffer , std::size_t buffer_size )
	{
		return AlignImp::align_imp<T>( buffer , buffer_size ) ;
	}

	/// \code
	/// Divides the number of bytes in a range to give the number
	/// of whole Ts.
	///
	/// \endcode
	template <typename T>
	inline
	constexpr std::size_t align_shift( std::size_t n )
	{
		return AlignImp::shift<T>( n ) ;
	}

	/// \code
	/// Rounds down the number of bytes in a range to give a number of
	/// bytes that will hold an exact number of Ts.
	///
	/// \endcode
	template <typename T>
	inline
	constexpr std::size_t align_mask( std::size_t n )
	{
		return AlignImp::mask<T>( n ) ;
	}
}

#endif
