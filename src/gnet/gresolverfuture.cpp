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
/// \file gresolverfuture.cpp
///

#include "gdef.h"
#include "gresolverfuture.h"
#include "gtest.h"
#include "gstr.h"
#include "gsleep.h"
#include "glog.h"
#include <cstring>
#include <cstdio>

GNet::ResolverFuture::ResolverFuture( const std::string & host , const std::string & service , int family ,
	bool dgram , bool for_async_hint ) :
		m_numeric_service(!service.empty() && G::Str::isNumeric(service)) ,
		m_socktype(dgram?SOCK_DGRAM:SOCK_STREAM) ,
		m_host(host) ,
		m_host_p(m_host.c_str()) ,
		m_service(service) ,
		m_service_p(m_service.c_str()) ,
		m_family(family) ,
		m_test_mode(for_async_hint&&G::Test::enabled("getaddrinfo-slow"))
{
	std::memset( &m_ai_hint , 0 , sizeof(m_ai_hint) ) ;
	m_ai_hint.ai_flags = AI_CANONNAME | // NOLINT
		( family == AF_UNSPEC ? AI_ADDRCONFIG : 0 ) |
		( m_numeric_service ? AI_NUMERICSERV : 0 ) ;
	m_ai_hint.ai_family = family ;
	m_ai_hint.ai_socktype = m_socktype ;
}

GNet::ResolverFuture::~ResolverFuture()
{
	if( m_ai )
		::freeaddrinfo( m_ai ) ; // documented as "thread-safe"
}

GNet::ResolverFuture & GNet::ResolverFuture::run() noexcept
{
	// worker thread - as simple as possible
	if( m_test_mode ) sleep( 10 ) ;
	m_rc = ::getaddrinfo( m_host_p , m_service_p , &m_ai_hint , &m_ai ) ;
	return *this ;
}

std::string GNet::ResolverFuture::failure() const
{
	std::stringstream ss ;
	if( m_numeric_service )
		ss << "no such " << ipvx() << "host: \"" << m_host << "\"" ;
	else
		ss << "no such " << ipvx() << "host or service: \"" << m_host << ":" << m_service << "\"" ;
	const char * reason = gai_strerror( m_rc ) ; // not portable, but see gdef.h
	if( reason && *reason )
		ss << " (" << G::Str::lower(G::Str::trimmed(std::string(reason)," .")) << ")" ;
	return ss.str() ;
}

std::string GNet::ResolverFuture::ipvx() const
{
	if( m_family == AF_UNSPEC ) return {} ;
	if( m_family == AF_INET ) return "ipv4 " ;
	return "ipv6 " ; // AF_INET6 possibly undefined
}

bool GNet::ResolverFuture::failed() const
{
	return m_rc != 0 || m_ai == nullptr || m_ai->ai_addr == nullptr || m_ai->ai_addrlen == 0 ;
}

std::string GNet::ResolverFuture::none() const
{
	return "no usable addresses returned for \"" + m_host + "\"" ;
}

bool GNet::ResolverFuture::fetch( Pair & pair ) const
{
	// fetch the first valid address/name pair
	for( const struct addrinfo * p = m_ai ; p ; p = p->ai_next )
	{
		socklen_t addrlen = static_cast<socklen_t>(p->ai_addrlen) ;
		if( Address::validData( p->ai_addr , addrlen ) )
		{
			Address address( p->ai_addr , addrlen ) ;
			std::string name( p->ai_canonname ? p->ai_canonname : "" ) ;
			pair = std::make_pair( address , name ) ;
			return true ;
		}
	}
	return false ;
}

bool GNet::ResolverFuture::fetch( List & list ) const
{
	// fetch all valid addresses
	bool got_one = false ;
	for( const struct addrinfo * p = m_ai ; p ; p = p->ai_next )
	{
		socklen_t addrlen = static_cast<socklen_t>(p->ai_addrlen) ;
		if( Address::validData( p->ai_addr , addrlen ) )
		{
			list.emplace_back( p->ai_addr , addrlen ) ;
			got_one = true ;
		}
	}
	return got_one ;
}

void GNet::ResolverFuture::get( List & list )
{
	if( failed() )
		m_reason = failure() ;
	else if( !fetch(list) )
		m_reason = none() ;
}

GNet::ResolverFuture::Pair GNet::ResolverFuture::get()
{
	Pair result( Address::defaultAddress() , std::string() ) ;
	if( failed() )
		m_reason = failure() ;
	else if( !fetch(result) )
		m_reason = none() ;
	return result ;
}

bool GNet::ResolverFuture::error() const
{
	return !m_reason.empty() ;
}

std::string GNet::ResolverFuture::reason() const
{
	return m_reason ;
}

