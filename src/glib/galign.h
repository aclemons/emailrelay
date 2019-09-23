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
/// \file galign.h
///

#ifndef G_ALIGN_H
#define G_ALIGN_H

#include "gdef.h"
#include <memory>

namespace G
{
	/// \namespace G::AlignImp
	/// An implementation namespace for G::align().
	///
	namespace AlignImp
	{
		template <unsigned int N>
		struct log2of /// Evaluates the number of bits in the template parameter N.
		{
			static g__constexpr unsigned int value = 1U + log2of<N/2>::value ;
		} ;

		template <>
		struct log2of<1U> /// Terminal specialisation of log2of<N>.
		{
			static g__constexpr unsigned int value = 0U ;
		} ;

		template <typename Talign, typename Tvalue>
		inline
		Tvalue mask( Tvalue n )
		{
			return n & (~(Tvalue(0))<<log2of<sizeof(Talign)>::value) ;
		}

		template <typename Talign, typename Tvalue>
		inline
		Tvalue shift( Tvalue n )
		{
			return n >> log2of<sizeof(Talign)>::value ;
		}

		template <typename Talign, typename Tchar>
		inline void * align_imp_local( const Tchar * p , const size_t n_in )
		{
			Tchar * out = reinterpret_cast<Tchar*>(mask<Talign>(reinterpret_cast<g_uintptr_t>(p)+sizeof(Talign)-1)) ;
			if( (out+sizeof(Talign)) > (p+n_in) ) out = nullptr ;
			return out ;
		}

		#if GCONFIG_HAVE_CXX_ALIGNMENT
		template <typename Talign, typename Tchar>
		inline void * align_imp_std( const Tchar * p , const size_t n_in )
		{
			void * vp = const_cast<Tchar*>(p) ;
			size_t n = n_in ;
			return std::align( alignof(Talign) , sizeof(Talign) , vp , n ) ;
		}
		#endif

		template <typename Talign, typename Tchar>
		inline void * align_imp( const Tchar * p , const size_t n_in )
		{
			#if GCONFIG_HAVE_CXX_ALIGNMENT
				return align_imp_std<Talign,Tchar>( p , n_in ) ;
			#else
				return align_imp_local<Talign,Tchar>( p , n_in ) ;
			#endif
		}
	}

/// Returns a pointer inside the given buffer that is aligned for
/// values of type T.
///
template <typename T>
inline void * align( const char * buffer , size_t buffer_size )
{
	namespace imp = AlignImp ;
	return imp::align_imp<T>( buffer , buffer_size ) ;
}

/// Returns a pointer inside the given unsigned-char buffer that
/// is aligned for values of type T.
///
template <typename T>
inline void * align( const unsigned char * buffer , size_t buffer_size )
{
	namespace imp = AlignImp ;
	return imp::align_imp<T>( buffer , buffer_size ) ;
}

}

#endif
