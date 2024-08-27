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
/// \file gssl_mbedtls_utils.h
///

#ifndef GSSL_MBEDTLS_UTILS_H
#define GSSL_MBEDTLS_UTILS_H

#include "gdef.h"
#include "gssl_mbedtls_headers.h"
#include "gstrmacros.h"
#include <cstddef> // std::nullptr_t

// macro magic to provide function-name/function-pointer arguments for call()
//
// in later versions of mbedtls v2 some functions like foo() have a preferred
// alternative form foo_ret() that returns an integer result code -- however
// in mbedtls v3 some of the foo_ret() functions are deprecated and foo()
// is now preferred
//
#ifdef FN
#undef FN
#endif
#define FN( fn ) nullptr,(#fn),(fn)
#if MBEDTLS_VERSION_NUMBER >= 0x02070000
#if MBEDTLS_VERSION_MAJOR >= 3
#define FN_RET( fn ) nullptr,(#fn),(fn)
#else
#define FN_RET( fn ) 0,(#fn),(G_STR_PASTE(fn,_ret))
#endif
#else
#define FN_RET( fn ) nullptr,(#fn),(fn)
#endif
#define FN_OK( ok , fn ) int(ok),(#fn),(fn)

namespace GSsl
{
	namespace MbedTls
	{
		// calls the given function with error checking -- overload for functions returning an integer status value
		template <typename F, typename... Args>
		void call( int ok , const char * fname , F fn , Args&&... args )
		{
			int rc = fn( std::forward<Args>(args)... ) ;
			if( rc != ok )
				throw Error( fname , rc ) ;
		}

		// calls the given function -- overload for functions returning void
		template <typename F, typename... Args>
		void call( std::nullptr_t , const char * /*fname*/ , F fn , Args&&... args )
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
