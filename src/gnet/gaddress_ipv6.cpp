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
//
// gaddress_ipv6.cpp
//

#include "gdef.h"
#include "gaddress.h"
#include "gaddress4.h"
#include "gaddress6.h"
#include <cstring>

namespace
{
	static bool is4( const sockaddr * p )
	{
		return p && p->sa_family == GNet::Address4::family() ;
	}
	static bool is4( const std::string & s )
	{
		std::string r ;
		return GNet::Address4::validString(s,&r) ;
	}
	static bool is4( const std::string & s , unsigned int )
	{
		std::string r ;
		return GNet::Address4::validStrings(s,"0",&r) ;
	}
}

bool GNet::Address::supports( Family )
{
	return true ;
}

GNet::Address GNet::Address::defaultAddress()
{
	return Address( Family::ipv4() , 0U ) ;
}

GNet::Address::Address( Family f , unsigned int port ) :
	m_4imp( f==Family::ipv4() ? new Address4(port) : nullptr ) ,
	m_6imp( f!=Family::ipv4() ? new Address6(port) : nullptr )
{
}

GNet::Address::Address( const AddressStorage & storage ) :
	m_4imp( is4(storage.p()) ? new Address4(storage.p(),storage.n()) : nullptr ) ,
	m_6imp( !is4(storage.p()) ? new Address6(storage.p(),storage.n()) : nullptr )
{
}

GNet::Address::Address( const sockaddr * addr , socklen_t len ) :
	m_4imp( is4(addr) ? new Address4(addr,len) : nullptr ) ,
	m_6imp( !is4(addr) ? new Address6(addr,len) : nullptr )
{
}

GNet::Address::Address( const std::string & s ) :
	m_4imp( is4(s) ? new Address4(s) : nullptr ) ,
	m_6imp( !is4(s) ? new Address6(s) : nullptr )
{
}

GNet::Address::Address( const std::string & s , unsigned int port ) :
	m_4imp( is4(s,port) ? new Address4(s,port) : nullptr ) ,
	m_6imp( !is4(s,port) ? new Address6(s,port) : nullptr )
{
}

GNet::Address::Address( const Address & other ) :
	m_4imp( other.m_4imp ? new Address4(*other.m_4imp) : nullptr ) ,
	m_6imp( other.m_6imp ? new Address6(*other.m_6imp) : nullptr )
{
}

GNet::Address::Address( Family f , unsigned int port , int loopback_overload ) :
	m_4imp( f==Family::ipv4() ? new Address4(port,loopback_overload) : nullptr ) ,
	m_6imp( f!=Family::ipv4() ? new Address6(port,loopback_overload) : nullptr )
{
}

GNet::Address GNet::Address::loopback( Family f , unsigned int port )
{
	return Address( f , port , 1 ) ;
}

void GNet::Address::operator=( const Address & addr )
{
	Address temp( addr ) ;
	std::swap( m_4imp , temp.m_4imp ) ;
	std::swap( m_6imp , temp.m_6imp ) ;
}

GNet::Address::~Address()
{
	delete m_4imp ;
	delete m_6imp ;
}

void GNet::Address::setPort( unsigned int port )
{
	m_4imp ? m_4imp->setPort(port) : m_6imp->setPort(port) ;
}

bool GNet::Address::isLoopback() const
{
	return
		( m_4imp && m_4imp->isLoopback() ) ||
		( m_6imp && m_6imp->isLoopback() ) ;
}

bool GNet::Address::isLocal( std::string & reason ) const
{
	return
		( m_4imp && m_4imp->isLocal(reason) ) ||
		( m_6imp && m_6imp->isLocal(reason) ) ;
}

bool GNet::Address::operator==( const Address & other ) const
{
	return
		( m_4imp && other.m_4imp && m_4imp->same(*other.m_4imp) ) ||
		( m_6imp && other.m_6imp && m_6imp->same(*other.m_6imp) ) ;
}

bool GNet::Address::operator!=( const Address & other ) const
{
	return !( *this == other ) ;
}

bool GNet::Address::sameHostPart( const Address & other ) const
{
	return
		( m_4imp && other.m_4imp && m_4imp->sameHostPart(*other.m_4imp) ) ||
		( m_6imp && other.m_6imp && m_6imp->sameHostPart(*other.m_6imp) ) ;
}

std::string GNet::Address::displayString() const
{
	return m_4imp ? m_4imp->displayString() : m_6imp->displayString() ;
}

std::string GNet::Address::hostPartString() const
{
	return m_4imp ? m_4imp->hostPartString() : m_6imp->hostPartString() ;
}

bool GNet::Address::validString( const std::string & s , std::string * reason_p )
{
	return Address4::validString( s , reason_p ) || Address6::validString( s , reason_p ) ;
}

bool GNet::Address::validStrings( const std::string & s1 , const std::string & s2 , std::string * reason_p )
{
	return Address4::validStrings( s1 , s2 , reason_p ) || Address6::validStrings( s1 , s2 , reason_p ) ;
}

sockaddr * GNet::Address::address()
{
	return m_4imp ? m_4imp->address() : m_6imp->address() ;
}

const sockaddr * GNet::Address::address() const
{
	return m_4imp ? m_4imp->address() : m_6imp->address() ;
}

socklen_t GNet::Address::length() const
{
	return m_4imp ? Address4::length() : Address6::length() ;
}

unsigned int GNet::Address::port() const
{
	return m_4imp ? m_4imp->port() : m_6imp->port() ;
}

unsigned long GNet::Address::scopeId( unsigned long default_ ) const
{
	return m_4imp ? default_ : m_6imp->scopeId() ;
}

bool GNet::Address::validPort( unsigned int port )
{
	return Address4::validPort( port ) ;
}

bool GNet::Address::validData( const sockaddr * addr , socklen_t len )
{
	return Address4::validData( addr , len ) || Address6::validData( addr , len ) ;
}

int GNet::Address::domain() const
{
	return m_4imp ? Address4::domain() : Address6::domain() ;
}

GNet::Address::Family GNet::Address::family() const
{
	return m_4imp ? Family::ipv4() : Family::ipv6() ;
}

G::StringArray GNet::Address::wildcards() const
{
	return m_4imp ? m_4imp->wildcards() : m_6imp->wildcards() ;
}

// ===

/// \class GNet::AddressStorageImp
/// A pimple-pattern implementation class used by GNet::AddressStorage.
///
class GNet::AddressStorageImp
{
public:
	Address6::union_type u ;
	socklen_t n ;
} ;

// ==

GNet::AddressStorage::AddressStorage() :
	m_imp(new AddressStorageImp)
{
	m_imp->n = sizeof(Address6::union_type) ;
}

GNet::AddressStorage::~AddressStorage()
{
	delete m_imp ;
}

sockaddr * GNet::AddressStorage::p1()
{
	return &(m_imp->u.general) ;
}

socklen_t * GNet::AddressStorage::p2()
{
	return &m_imp->n ;
}

const sockaddr * GNet::AddressStorage::p() const
{
	return &m_imp->u.general ;
}

socklen_t GNet::AddressStorage::n() const
{
	return m_imp->n ;
}

// ==

// fallback implementation for inet_pton() -- see GCONFIG_HAVE_INET_PTON in gdef.h
//
int GNet::inet_pton_imp( int f , const char * p , void * result )
{
	if( p == nullptr || result == nullptr ) return 0 ; // just in case
	struct addrinfo ai_hint ;
	std::memset( &ai_hint , 0 , sizeof(ai_hint) ) ;
	ai_hint.ai_family = f ;
	ai_hint.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV ;
	struct addrinfo * ai_p = nullptr ;
	int rc = getaddrinfo( p , nullptr , &ai_hint , &ai_p ) ;
	bool ok = rc == 0 && ai_p != nullptr ;
	if( ok && ai_p->ai_addr == nullptr ) { freeaddrinfo(ai_p) ; return 1 ; } // just in case
	if( ok )
	{
		struct sockaddr * sa_p = ai_p->ai_addr ;
		if( ai_p->ai_family == AF_INET )
		{
			struct sockaddr_in sa = *reinterpret_cast<struct sockaddr_in*>(sa_p) ;
			*reinterpret_cast<struct in_addr*>(result) = sa.sin_addr ;
		}
		else if( ai_p->ai_family == AF_INET6 )
		{
			struct sockaddr_in6 sa = *reinterpret_cast<struct sockaddr_in6*>(sa_p) ;
			*reinterpret_cast<struct in6_addr*>(result) = sa.sin6_addr ;
		}
		else
			ok = false ;
		freeaddrinfo( ai_p ) ;
	}
	return ok ? 1 : 0 ;
}

#if ! GCONFIG_HAVE_INET_NTOP
// fallback implementation for inet_ntop() -- see GCONFIG_HAVE_INET_NTOP in gdef.h
const char * GNet::inet_ntop_imp( int f , void * ap , char * buffer , size_t n )
{
	std::string s ;
	if( f == AF_INET )
	{
		std::ostringstream ss ;
		struct in_addr a = *reinterpret_cast<struct in_addr*>(ap) ;
		ss << inet_ntoa( a ) ; // ignore warnings - this is not used if inet_ntop is available
		s = ss.str() ;
	}
	else if( f == AF_INET6 )
	{
		struct in6_addr a = *reinterpret_cast<struct in6_addr*>(ap) ;
		std::ostringstream ss ;
		const char * sep = ":" ;
		const char * hexmap = "0123456789abcdef" ;
		for( int i = 0 ; i < 16 ; i++ , sep = *sep ? "" : ":" )
		{
			unsigned int n = static_cast<unsigned int>(a.s6_addr[i]) % 256U ;
			ss << sep << hexmap[(n>>4U)%16U] << hexmap[(n&15U)%16U] ;
		}
		ss << ":" ;
		s = ss.str() ;
		for( std::string::size_type pos = s.find(":0") ; pos != std::string::npos ; pos = s.find(":0",pos) )
		{
			if( s.find(":0:",pos) == std::string::npos )
				s.erase( pos+1U , 1U ) ;
			else
				pos++ ;
		}
		std::string run = ":0:0:0:0:0:0:0:0:" ;
		while( run.length() >= 3U )
		{
			std::string::size_type pos = s.find( run ) ;
			if( pos != std::string::npos )
			{
				std::string::size_type r = 2U ;
				if( pos == 0U ) r++ ;
				if( (pos + run.length()) == s.length() ) r++ ;
				s.replace( pos , run.length() , std::string("::::").substr(0U,r) ) ;
				break ;
			}
			run.erase( 0U , 2U ) ;
		}
		s.erase( 0U , 1U ) ;
		s.erase( s.length()-1U , 1U ) ;
	}
	else
	{
		return nullptr ;
	}
	if( n <= s.length() ) return nullptr ;
	std::strncpy( buffer , s.c_str() , n ) ;
	return buffer ;
}
#endif

