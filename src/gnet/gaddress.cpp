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
/// \file gaddress.cpp
///

#include "gdef.h"
#include "gaddress4.h"
#include "gaddress6.h"
#include "gaddresslocal.h"
#include "gaddress.h"
#include "gstr.h"
#include "gassert.h"
#include <algorithm> // std::swap()
#include <utility> // std::swap()
#include <sstream>
#include <cstring>

bool GNet::Address::supports( Family f ) noexcept
{
	if( f == Address::Family::ipv4 && Address4::af() == 0 )
		return false ; // fwiw
	else if( f == Address::Family::ipv6 && Address6::af() == 0 )
		return false ; // fwiw
	else if( f == Address::Family::local && AddressLocal::af() == 0 )
		return false ;
	else
		return true ;
}

bool GNet::Address::supports( int af , int ) noexcept
{
	return af == Address4::af() || af == Address6::af() || af == AddressLocal::af() ;
}

bool GNet::Address::supports( const Address::Domain & , int domain ) noexcept
{
	return domain == Address4::domain() || domain == Address6::domain() || domain == AddressLocal::domain() ;
}

GNet::Address::Address( Family f , unsigned int port )
{
	if( Address4::af() && f == Address::Family::ipv4 )
		m_ipv4 = std::make_unique<Address4>( port ) ;
	else if( Address6::af() && f == Address::Family::ipv6 )
		m_ipv6 = std::make_unique<Address6>( port ) ;
	else if( AddressLocal::af() && f == Address::Family::local )
		m_local = std::make_unique<AddressLocal>( port ) ;
	else
		throw Address::BadFamily() ;
}

GNet::Address::Address( const sockaddr * addr , socklen_t len , bool ipv6_scope_id_fixup )
{
	if( addr == nullptr || len < static_cast<socklen_t>(sizeof(sockaddr::sa_family)) )
		throw Address::Error() ;
	else if( addr->sa_family == 0 )
		throw Address::BadFamily() ;
	else if( Address4::af() && addr->sa_family == Address4::af() )
		m_ipv4 = std::make_unique<Address4>( addr , len ) ;
	else if( Address6::af() && addr->sa_family == Address6::af() )
		m_ipv6 = std::make_unique<Address6>( addr , len , ipv6_scope_id_fixup ) ;
	else if( AddressLocal::af() && addr->sa_family == AddressLocal::af() )
		m_local = std::make_unique<AddressLocal>( addr , len ) ;
	else
		throw Address::BadFamily() ;
}

GNet::Address::Address( const sockaddr * addr , socklen_t len ) :
	Address(addr,len,false)
{
}

GNet::Address::Address( const AddressStorage & storage ) :
	Address(storage.p(),storage.n(),false)
{
}

GNet::Address::Address( const std::string & s , bool with_local )
{
	if( s.empty() )
		throw Address::Error( "empty string" ) ;

	std::string r1 ;
	std::string r2 ;
	if( with_local && AddressLocal::af() && isFamilyLocal(s) )
		m_local = std::make_unique<AddressLocal>( s ) ;
	else if( Address4::af() && Address4::validString(s,&r1) )
		m_ipv4 = std::make_unique<Address4>( s ) ;
	else if( Address6::af() && Address6::validString(s,&r2) )
		m_ipv6 = std::make_unique<Address6>( s ) ;
	else
		throw Address::Error( r1 , r1==r2?std::string():r2 , G::Str::printable(s) ) ;
}

GNet::Address::Address( const std::string & host_part , const std::string & port_part )
{
	if( host_part.empty() )
		throw Address::Error( "empty string" ) ;

	std::string r1 ;
	std::string r2 ;
	if( AddressLocal::af() && isFamilyLocal( host_part ) )
		m_local = std::make_unique<AddressLocal>( host_part ) ;
	else if( Address4::af() && Address4::validStrings(host_part,port_part,&r1) )
		m_ipv4 = std::make_unique<Address4>( host_part , port_part ) ;
	else if( Address6::af() && Address6::validStrings(host_part,port_part,&r2) )
		m_ipv6 = std::make_unique<Address6>( host_part , port_part ) ;
	else
		throw Address::Error( r1 , r1==r2?std::string():r2 , G::Str::printable(host_part) , G::Str::printable(port_part) ) ;
}

GNet::Address::Address( const std::string & host_part , unsigned int port ) :
	Address(host_part,G::Str::fromUInt(port))
{
}

GNet::Address::Address( Family f , unsigned int port , int loopback_overload )
{
	if( Address4::af() && f == Address::Family::ipv4 )
		m_ipv4 = std::make_unique<Address4>( port , loopback_overload ) ;
	else if( Address6::af() && f == Address::Family::ipv6 )
		m_ipv6 = std::make_unique<Address6>( port , loopback_overload ) ;
	else if( AddressLocal::af() && f == Address::Family::local )
		m_local = std::make_unique<AddressLocal>( port , loopback_overload ) ;
	else
		throw Address::BadFamily() ;
}

GNet::Address::Address( const Address & other )
{
	G_ASSERT( other.m_ipv4 || other.m_ipv6 || other.m_local ) ;
	if( other.m_ipv4 )
		m_ipv4 = std::make_unique<Address4>( *other.m_ipv4 ) ;
	if( other.m_ipv6 )
		m_ipv6 = std::make_unique<Address6>( *other.m_ipv6 ) ;
	if( other.m_local )
		m_local = std::make_unique<AddressLocal>( *other.m_local ) ;
}

GNet::Address::Address( Address && other ) noexcept
= default ;

GNet::Address::~Address()
= default;

void GNet::Address::swap( Address & other ) noexcept
{
	using std::swap ;
	swap( m_ipv4 , other.m_ipv4 ) ;
	swap( m_ipv6 , other.m_ipv6 ) ;
	swap( m_local , other.m_local ) ;
}

GNet::Address & GNet::Address::operator=( const Address & other )
{
	Address(other).swap( *this ) ;
	return *this ;
}

GNet::Address & GNet::Address::operator=( Address && other ) noexcept
= default ;

GNet::Address GNet::Address::parse( const std::string & s )
{
	return { s , true } ;
}

GNet::Address GNet::Address::parse( const std::string & s , Address::NotLocal )
{
	return { s , false } ;
}

GNet::Address GNet::Address::parse( const std::string & host_part , unsigned int port )
{
	return { host_part , port } ;
}

#ifndef G_LIB_SMALL
GNet::Address GNet::Address::parse( const std::string & host_part , const std::string & port_part )
{
	return { host_part , port_part } ;
}
#endif

bool GNet::Address::isFamilyLocal( const std::string & s ) noexcept
{
	return supports( Family::local ) && !s.empty() && s[0] == '/' ;
}

GNet::Address GNet::Address::defaultAddress()
{
	return { Family::ipv4 , 0U } ;
}

GNet::Address GNet::Address::loopback( Family f , unsigned int port )
{
	return { f , port , 1 } ;
}

GNet::Address::operator G::BasicAddress() const
{
	return G::BasicAddress( displayString() ) ;
}

GNet::Address & GNet::Address::setPort( unsigned int port )
{
	G_ASSERT( m_ipv4 || m_ipv6 || m_local ) ;
	if( m_ipv4 ) m_ipv4->setPort( port ) ;
	if( m_ipv6 ) m_ipv6->setPort( port ) ;
	if( m_local ) m_local->setPort( port ) ;
	return *this ;
}

#ifndef G_LIB_SMALL
bool GNet::Address::setZone( const std::string & ipv6_zone )
{
	G_ASSERT( m_ipv4 || m_ipv6 || m_local ) ;
	if( m_ipv4 ) m_ipv4->setZone( ipv6_zone ) ;
	if( m_ipv6 ) m_ipv6->setZone( ipv6_zone ) ;
	if( m_local ) m_local->setZone( ipv6_zone ) ;
	return true ;
}
#endif

GNet::Address & GNet::Address::setScopeId( unsigned long ipv6_scope_id )
{
	G_ASSERT( m_ipv4 || m_ipv6 || m_local ) ;
	if( m_ipv4 ) m_ipv4->setScopeId( ipv6_scope_id ) ;
	if( m_ipv6 ) m_ipv6->setScopeId( ipv6_scope_id ) ;
	if( m_local ) m_local->setScopeId( ipv6_scope_id ) ;
	return *this ;
}

unsigned int GNet::Address::bits() const
{
	G_ASSERT( m_ipv4 || m_ipv6 || m_local ) ;
	if( m_ipv4 ) return m_ipv4->bits() ;
	if( m_ipv6 ) return m_ipv6->bits() ;
	if( m_local ) return m_local->bits() ;
	return 0U ;
}

bool GNet::Address::isLoopback() const
{
	G_ASSERT( m_ipv4 || m_ipv6 || m_local ) ;
	return
		( m_ipv4 && m_ipv4->isLoopback() ) ||
		( m_ipv6 && m_ipv6->isLoopback() ) ||
		( m_local && m_local->isLoopback() ) ;
}

bool GNet::Address::isLocal( std::string & reason ) const
{
	G_ASSERT( m_ipv4 || m_ipv6 || m_local ) ;
	return
		( m_ipv4 && m_ipv4->isLocal(reason) ) ||
		( m_ipv6 && m_ipv6->isLocal(reason) ) ||
		( m_local && m_local->isLocal(reason) ) ;
}

bool GNet::Address::isLinkLocal() const
{
	G_ASSERT( m_ipv4 || m_ipv6 || m_local ) ;
	return
		( m_ipv4 && m_ipv4->isLinkLocal() ) ||
		( m_ipv6 && m_ipv6->isLinkLocal() ) ||
		( m_local && m_local->isLinkLocal() ) ;
}

#ifndef G_LIB_SMALL
bool GNet::Address::isMulticast() const
{
	G_ASSERT( m_ipv4 || m_ipv6 || m_local ) ;
	return
		( m_ipv4 && m_ipv4->isMulticast() ) ||
		( m_ipv6 && m_ipv6->isMulticast() ) ||
		( m_local && m_local->isMulticast() ) ;
}
#endif

bool GNet::Address::isUniqueLocal() const
{
	G_ASSERT( m_ipv4 || m_ipv6 || m_local ) ;
	return
		( m_ipv4 && m_ipv4->isUniqueLocal() ) ||
		( m_ipv6 && m_ipv6->isUniqueLocal() ) ||
		( m_local && m_local->isUniqueLocal() ) ;
}

bool GNet::Address::isAny() const
{
	G_ASSERT( m_ipv4 || m_ipv6 || m_local ) ;
	return
		( m_ipv4 && m_ipv4->isAny() ) ||
		( m_ipv6 && m_ipv6->isAny() ) ||
		( m_local && m_local->isAny() ) ;
}

bool GNet::Address::is4() const noexcept
{
	return !!m_ipv4 ;
}

bool GNet::Address::is6() const noexcept
{
	return !!m_ipv6 ;
}

bool GNet::Address::same( const Address & other , bool ipv6_compare_with_scope ) const
{
	G_ASSERT( m_ipv4 || m_ipv6 || m_local ) ;
	return
		( m_ipv4 && other.m_ipv4 && m_ipv4->same(*other.m_ipv4,ipv6_compare_with_scope) ) ||
		( m_ipv6 && other.m_ipv6 && m_ipv6->same(*other.m_ipv6,ipv6_compare_with_scope) ) ||
		( m_local && other.m_local && m_local->same(*other.m_local,ipv6_compare_with_scope) ) ;
}

bool GNet::Address::operator==( const Address & other ) const
{
	G_ASSERT( m_ipv4 || m_ipv6 || m_local ) ;
	return
		( m_ipv4 && other.m_ipv4 && m_ipv4->same(*other.m_ipv4) ) ||
		( m_ipv6 && other.m_ipv6 && m_ipv6->same(*other.m_ipv6) ) ||
		( m_local && other.m_local && m_local->same(*other.m_local) ) ;
}

bool GNet::Address::operator!=( const Address & other ) const
{
	return !( *this == other ) ;
}

#ifndef G_LIB_SMALL
bool GNet::Address::sameHostPart( const Address & other ) const
{
	G_ASSERT( m_ipv4 || m_ipv6 || m_local ) ;
	return
		( m_ipv4 && other.m_ipv4 && m_ipv4->sameHostPart(*other.m_ipv4) ) ||
		( m_ipv6 && other.m_ipv6 && m_ipv6->sameHostPart(*other.m_ipv6) ) ||
		( m_local && other.m_local && m_local->sameHostPart(*other.m_local) ) ;
}
#endif

std::string GNet::Address::displayString( bool ipv6_with_scope_id ) const
{
	G_ASSERT( m_ipv4 || m_ipv6 || m_local ) ;
	if( m_ipv4 ) return m_ipv4->displayString( ipv6_with_scope_id ) ;
	if( m_ipv6 ) return m_ipv6->displayString( ipv6_with_scope_id ) ;
	if( m_local ) return m_local->displayString( ipv6_with_scope_id ) ;
	return {} ;
}

std::string GNet::Address::hostPartString() const
{
	G_ASSERT( m_ipv4 || m_ipv6 || m_local ) ;
	if( m_ipv4 ) return m_ipv4->hostPartString() ;
	if( m_ipv6 ) return m_ipv6->hostPartString() ;
	if( m_local ) return m_local->hostPartString() ;
	return {} ;
}

std::string GNet::Address::queryString() const
{
	G_ASSERT( m_ipv4 || m_ipv6 || m_local ) ;
	if( m_ipv4 ) return m_ipv4->queryString() ;
	if( m_ipv6 ) return m_ipv6->queryString() ;
	if( m_local ) return m_local->queryString() ;
	return {} ;
}

bool GNet::Address::validString( const std::string & s , std::string * reason_p )
{
	return
		Address4::validString( s , reason_p ) ||
		Address6::validString( s , reason_p ) ||
		AddressLocal::validString( s , reason_p ) ;
}

bool GNet::Address::validString( const std::string & s , NotLocal , std::string * reason_p )
{
	return
		Address4::validString( s , reason_p ) ||
		Address6::validString( s , reason_p ) ;
}

bool GNet::Address::validStrings( const std::string & s1 , const std::string & s2 , std::string * reason_p )
{
	return
		Address4::validStrings( s1 , s2 , reason_p ) ||
		Address6::validStrings( s1 , s2 , reason_p ) ||
		AddressLocal::validStrings( s1 , s2 , reason_p ) ;
}

#ifndef G_LIB_SMALL
sockaddr * GNet::Address::address()
{
	G_ASSERT( m_ipv4 || m_ipv6 || m_local ) ;
	if( m_ipv4 ) return m_ipv4->address() ;
	if( m_ipv6 ) return m_ipv6->address() ;
	if( m_local ) return m_local->address() ;
	return nullptr ;
}
#endif

const sockaddr * GNet::Address::address() const
{
	G_ASSERT( m_ipv4 || m_ipv6 || m_local ) ;
	if( m_ipv4 ) return m_ipv4->address() ;
	if( m_ipv6 ) return m_ipv6->address() ;
	if( m_local ) return m_local->address() ;
	return nullptr ;
}

socklen_t GNet::Address::length() const
{
	G_ASSERT( m_ipv4 || m_ipv6 || m_local ) ;
	if( m_ipv4 ) return Address4::length() ;
	if( m_ipv6 ) return Address6::length() ;
	if( m_local ) return m_local->length() ;
	return 0 ;
}

unsigned int GNet::Address::port() const
{
	G_ASSERT( m_ipv4 || m_ipv6 || m_local ) ;
	if( m_ipv4 ) return m_ipv4->port() ;
	if( m_ipv6 ) return m_ipv6->port() ;
	if( m_local ) return m_local->port() ;
	return 0 ;
}

unsigned long GNet::Address::scopeId( unsigned long default_ ) const
{
	G_ASSERT( m_ipv4 || m_ipv6 || m_local ) ;
	if( m_ipv4 ) return m_ipv4->scopeId( default_ ) ;
	if( m_ipv6 ) return m_ipv6->scopeId( default_ ) ;
	if( m_local ) return m_local->scopeId( default_ ) ;
	return default_ ;
}

bool GNet::Address::validPort( unsigned int port )
{
	return Address4::validPort( port ) ;
}

bool GNet::Address::validData( const sockaddr * addr , socklen_t len )
{
	return
		Address4::validData( addr , len ) ||
		Address6::validData( addr , len ) ||
		AddressLocal::validData( addr , len ) ;
}

int GNet::Address::domain( Family family ) noexcept
{
	if( family == Family::ipv4 ) return Address4::domain() ;
	if( family == Family::ipv6 ) return Address6::domain() ;
	if( family == Family::local ) return AddressLocal::domain() ;
	return 0 ;
}

GNet::Address::Family GNet::Address::family() const noexcept
{
	if( m_ipv4 ) return Family::ipv4 ;
	if( m_ipv6 ) return Family::ipv6 ;
	if( m_local ) return Family::local ;
	return Family::ipv4 ;
}

int GNet::Address::af() const noexcept
{
	if( m_ipv4 ) return Address4::af() ;
	if( m_ipv6 ) return Address6::af() ;
	if( m_local ) return AddressLocal::af() ;
	return 0 ;
}

G::StringArray GNet::Address::wildcards() const
{
	G_ASSERT( m_ipv4 || m_ipv6 || m_local ) ;
	if( m_ipv4 ) return m_ipv4->wildcards() ;
	if( m_ipv6 ) return m_ipv6->wildcards() ;
	if( m_local ) return m_local->wildcards() ;
	return {} ;
}

// ===

//| \class GNet::AddressStorageImp
/// A pimple-pattern implementation class used by GNet::AddressStorage.
///
class GNet::AddressStorageImp
{
public:
	sockaddr_storage u ;
	socklen_t n ;
} ;

// ==

GNet::AddressStorage::AddressStorage() :
	m_imp(std::make_unique<AddressStorageImp>())
{
	static_assert( sizeof(Address4::sockaddr_type) <= sizeof(sockaddr_storage) , "" ) ;
	static_assert( sizeof(Address6::sockaddr_type) <= sizeof(sockaddr_storage) , "" ) ;
	static_assert( sizeof(AddressLocal::sockaddr_type) <= sizeof(sockaddr_storage) , "" ) ;

	static_assert( alignof(Address4::sockaddr_type) <= alignof(sockaddr_storage) , "" ) ;
	static_assert( alignof(Address6::sockaddr_type) <= alignof(sockaddr_storage) , "" ) ;
	static_assert( alignof(AddressLocal::sockaddr_type) <= alignof(sockaddr_storage) , "" ) ;

	m_imp->n = sizeof( sockaddr_storage ) ;
}

GNet::AddressStorage::~AddressStorage()
= default ;

sockaddr * GNet::AddressStorage::p1()
{
	return reinterpret_cast<sockaddr*>(&(m_imp->u)) ;
}

socklen_t * GNet::AddressStorage::p2()
{
	return &m_imp->n ;
}

const sockaddr * GNet::AddressStorage::p() const
{
	return reinterpret_cast<const sockaddr*>(&(m_imp->u)) ;
}

socklen_t GNet::AddressStorage::n() const
{
	return m_imp->n ;
}

// ==

#if ! GCONFIG_HAVE_INET_PTON
// fallback implementation for inet_pton() using getaddrinfo() -- see gdef.h
#ifndef G_LIB_SMALL
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
			struct sockaddr_in sa ;
			std::memcpy( &sa , sa_p , sizeof(sa) ) ;
			std::memcpy( result , &sa.sin_addr , sizeof(sa.sin_addr) ) ;
		}
		else if( ai_p->ai_family == AF_INET6 )
		{
			struct sockaddr_in6 sa ;
			std::memcpy( &sa , sa_p , sizeof(sa) ) ;
			std::memcpy( result , &sa.sin6_addr , sizeof(sa.sin6_addr) ) ;
		}
		else
		{
			ok = false ;
		}
		freeaddrinfo( ai_p ) ;
	}
	return ok ? 1 : 0 ;
}
#endif
#endif

#if ! GCONFIG_HAVE_INET_NTOP
// fallback implementation for inet_ntop() using inet_ntoa() for ipv4 and by hand for ipv6 -- see gdef.h
#ifndef G_LIB_SMALL
const char * GNet::inet_ntop_imp( int f , void * ap , char * buffer , std::size_t n )
{
	std::string s ;
	if( f == AF_INET )
	{
		std::ostringstream ss ;
		struct in_addr a ;
		std::memcpy( &a , ap , sizeof(a) ) ;
		ss << inet_ntoa( a ) ; // ignore warnings - this code is not used if inet_ntop is available
		s = ss.str() ;
	}
	else if( f == AF_INET6 )
	{
		struct in6_addr a ;
		std::memcpy( &a , ap , sizeof(a) ) ;
		std::ostringstream ss ;
		const char * sep = ":" ;
		const char * hexmap = "0123456789abcdef" ;
		for( int i = 0 ; i < 16 ; i++ , sep = *sep ? "" : ":" ) // sep alternates
		{
			unsigned int nn = static_cast<unsigned int>(a.s6_addr[i]) % 256U ;
			ss << sep << hexmap[(nn>>4U)%16U] << hexmap[(nn&15U)%16U] ;
		}
		ss << ":" ;
		// eg. ":0001:0002:0000:0000:0005:0006:dead:beef:"
		s = ss.str() ;
		for( std::string::size_type pos = s.find(":0") ; pos != std::string::npos ; pos = s.find(":0",pos) )
		{
			pos += 1U ;
			while( s.at(pos) == '0' && s.at(pos+1U) != ':' )
				s.erase( pos , 1U ) ;
		}
		// eg. ":1:2:0:0:5:6:dead:beef:"
		std::string run = ":0:0:0:0:0:0:0:0:" ;
		while( run.length() >= 5U ) // (single zero fields are not elided)
		{
			std::string::size_type pos = s.find( run ) ;
			if( pos != std::string::npos )
			{
				std::string::size_type r = 2U ; // ":1:0:0:2:" -> ":1::2:"
				if( pos == 0U ) r++ ; // ":0:0:1:2:" -> ":::1:2:"
				if( (pos + run.length()) == s.length() ) r++ ; // ":1:0:0:" -> ":1:::", ":0:0:0:" -> "::::"
				s.replace( pos , run.length() , std::string("::::").substr(0U,r) ) ;
				break ;
			}
			run.erase( 0U , 2U ) ;
		}
		// eg. ":1:2::5:6:dead:beef:"
		G_ASSERT( s.length() > 2U ) ;
		s.erase( 0U , 1U ) ;
		s.erase( s.length()-1U , 1U ) ;
		// eg. "1:2::5:6:dead:beef"
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
#endif

