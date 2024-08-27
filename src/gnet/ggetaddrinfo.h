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
/// \file ggetaddrinfo.h
///

#ifndef G_GETADDRINFO_H
#define G_GETADDRINFO_H

#include "gdef.h"
#ifdef G_WINDOWS
#include "gnowide.h"
#include "gstr.h"
#include <cstdlib> // std::malloc()
#include <cstring> // _strdup(), std::memcpy()
#include <stdexcept>
namespace GNet
{
	namespace GetAddrInfo
	{
		inline int getaddrinfo( const char * host_in , const char * service_in , addrinfo * hint_in , addrinfo ** results_out )
		{
			using ADDRINFO_type = G::nowide::ADDRINFO_type ;
			std::string host( host_in ? host_in : "" ) ;
			std::string service( service_in ? service_in : "" ) ;
			ADDRINFO_type win_hint {} ;
			if( hint_in )
			{
				win_hint.ai_flags = hint_in->ai_flags ;
				win_hint.ai_family = hint_in->ai_family ;
				win_hint.ai_socktype = hint_in->ai_socktype ;
				win_hint.ai_protocol = hint_in->ai_protocol ;
				win_hint.ai_addrlen = hint_in->ai_addrlen ;
				win_hint.ai_addr = hint_in->ai_addr ;
				win_hint.ai_next = nullptr ;
				win_hint.ai_canonname = nullptr ;
			}
			*results_out = nullptr ;
			ADDRINFO_type * win_results = nullptr ;
			auto rc = G::nowide::getAddrInfo( host , service , hint_in ? &win_hint : nullptr , &win_results ) ;
			if( rc == 0 )
			{
				addrinfo * last_result = nullptr ;
				for( ADDRINFO_type * p = win_results ; p ; p = p->ai_next )
				{
					addrinfo * result = new addrinfo ;
					std::memset( result , 0 , sizeof(*result) ) ;
					result->ai_flags = p->ai_flags ;
					result->ai_family = p->ai_family ;
					result->ai_socktype = p->ai_socktype ;
					result->ai_protocol = p->ai_protocol ;
					result->ai_addrlen = p->ai_addrlen ;
					result->ai_canonname = nullptr ;
					std::string canonical_name = G::nowide::canonicalName( *p ) ;
					result->ai_addr = static_cast<sockaddr*>( std::malloc( p->ai_addrlen ) ) ;
					if( result->ai_addr == nullptr )
						throw std::bad_alloc() ;
					std::memcpy( result->ai_addr , p->ai_addr , p->ai_addrlen ) ;
					if( !canonical_name.empty() )
						result->ai_canonname = _strdup( canonical_name.c_str() ) ;
					result->ai_next = nullptr ;

					if( *results_out == nullptr )
						*results_out = result ;
					if( last_result )
						last_result->ai_next = result ;
					last_result = result ;
				}
			}
			G::nowide::freeAddrInfo( win_results ) ;
			return rc ;
		}
		inline void freeaddrinfo( addrinfo * results ) noexcept
		{
			for( addrinfo * p = results ; p ; )
			{
				std::free( p->ai_canonname ) ;
				std::free( p->ai_addr ) ;
				auto next = p->ai_next ;
				delete p ;
				p = next ;
			}
		}
		inline const char * gai_strerror( int rc ) noexcept
		{
			if( rc == EAI_AGAIN ) return "temporary failure in name resolution" ;
			if( rc == EAI_BADFLAGS ) return "invalid value in ai_flags" ;
			if( rc == EAI_FAIL ) return "nonrecoverable failure in name resolution" ;
			if( rc == EAI_FAMILY ) return "ai_family not supported" ;
			if( rc == EAI_MEMORY ) return "memory allocation failure" ;
			if( rc == EAI_NONAME ) return "name does not resolve" ;
			if( rc == EAI_SERVICE ) return "invalid service" ;
			if( rc == EAI_SOCKTYPE ) return "ai_socktype not supported" ;
			return "getaddrinfo error" ;
		}
		static_assert( EAI_AGAIN == WSATRY_AGAIN , "" ) ;
		static_assert( EAI_BADFLAGS == WSAEINVAL , "" ) ;
		static_assert( EAI_FAIL == WSANO_RECOVERY , "" ) ;
		static_assert( EAI_FAMILY == WSAEAFNOSUPPORT , "" ) ;
		static_assert( EAI_MEMORY == WSA_NOT_ENOUGH_MEMORY , "" ) ;
		static_assert( EAI_NONAME == WSAHOST_NOT_FOUND , "" ) ;
		static_assert( EAI_SERVICE == WSATYPE_NOT_FOUND , "" ) ;
		static_assert( EAI_SOCKTYPE == WSAESOCKTNOSUPPORT , "" ) ;
	}
}
#else
namespace GNet
{
	namespace GetAddrInfo
	{
		using ::getaddrinfo ;
		using ::freeaddrinfo ;
		using ::gai_strerror ;
	}
}
#endif

#endif
