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
/// \file gssl_mbedtls_utils.h
///

#ifndef GSSL_MBEDTLS_UTILS_H
#define GSSL_MBEDTLS_UTILS_H

#include "gdef.h"
#include "gssl_mbedtls_headers.h"
#include "gstrmacros.h"

// macro magic to provide function-name/function-pointer arguments for call()
#ifdef FN
#undef FN
#endif
#define FN( fn ) (#fn),(fn)

// in newer versions of the mbedtls library hashing functions like mbed_whatever_ret()
// returning an integer are preferred, compared to mbed_whatever() returning void --
// except in version 3 some functions go back to a void return
#if MBEDTLS_VERSION_NUMBER >= 0x02070000
#define FN_RET( fn ) (#fn),(G_STR_PASTE(fn,_ret))
#if MBEDTLS_VERSION_MAJOR >= 3
#define FN_RETv3( fn ) (#fn),(fn)
#else
#define FN_RETv3( fn ) (#fn),(G_STR_PASTE(fn,_ret))
#endif
#else
#define FN_RET( fn ) (#fn),(fn)
#define FN_RETv3( fn ) (#fn),(fn)
#endif

namespace GSsl
{
	namespace MbedTls
	{
		// calls the given function with error checking -- overload for functions returning int
		template <typename F, typename... Args>
		typename std::enable_if< !std::is_same<void,typename std::result_of<F(Args...)>::type>::value >::type
		call( const char * fname , F fn , Args&&... args )
		{
			int rc = fn( std::forward<Args>(args)... ) ;
			if( rc )
				throw Error( fname , rc ) ;
		}

		// calls the given function -- overload for functions returning void
		template <typename F, typename... Args>
		typename std::enable_if< std::is_same<void,typename std::result_of<F(Args...)>::type>::value >::type
		call( const char * , F fn , Args&&... args )
		{
			fn( std::forward<Args>(args)... ) ;
		}

		// calls mbedtls_pk_parse_key() with or without the new rng parameters
		using old_fn = int (*)( mbedtls_pk_context * c , const unsigned char * k , std::size_t ks ,
			const unsigned char * p , std::size_t ps ) ;
		using new_fn = int (*)( mbedtls_pk_context* c , const unsigned char* k , std::size_t ks ,
			const unsigned char * p , std::size_t ps ,
			int (*r)(void*,unsigned char*,std::size_t) , void* rp ) ;
		inline int call_fn( old_fn fn ,
			mbedtls_pk_context * c , const unsigned char * k , std::size_t ks ,
			const unsigned char * p , std::size_t ps ,
			int (*)(void*,unsigned char*,std::size_t) , void * )
		{
			return fn( c , k , ks , p , ps ) ;
		}
		inline int call_fn( new_fn fn ,
			mbedtls_pk_context * c , const unsigned char * k , std::size_t ks ,
			const unsigned char * p , std::size_t ps ,
			int (*r)(void*,unsigned char*,std::size_t) , void * rp )
		{
			return fn( c , k , ks , p , ps , r , rp ) ;
		}

		template <typename T>
		struct X /// Initialises and frees an mbedtls object on construction and destruction.
		{
			X( void (*init)(T*) , void (*free)(T*) ) : m_free(free) { init(&x) ; }
			~X() { m_free(&x) ; }
			T * ptr() { return &x ; }
			const T * ptr() const { return &x ; }
			T x ;
			void (*m_free)(T*) ;
			X * operator&() = delete ;
			const X * operator&() const = delete ;
			X( const X<T> & ) = delete ;
			X( X<T> && ) = delete ;
			X<T> & operator=( const X<T> & ) = delete ;
			X<T> & operator=( X<T> && ) = delete ;
		} ;
	}
}

#endif
