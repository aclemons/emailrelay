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
/// \file gaddress_ipv4.cpp
///

#include "gdef.h"
#include "gaddress4.h"
#include "gaddress.h"
#include "gassert.h"
#include <algorithm> // std::swap()
#include <utility> // std::swap()
#include <cstring>

namespace GNet
{
	class Address6
	{
	} ;
	namespace AddressImp
	{
		void check( GNet::Address::Family f )
		{
			if( !Address::supports(f) )
				throw Address::BadFamily() ;
		}
	}
}

bool GNet::Address::supports( Family f )
{
	return f == Family::ipv4 ;
}

bool GNet::Address::supports( int af , int )
{
	return af == AF_INET ;
}

GNet::Address GNet::Address::defaultAddress()
{
	return Address( Family::ipv4 , 0U ) ;
}

GNet::Address::Address( Family f , unsigned int port ) :
	m_4imp(std::make_unique<Address4>(port))
{
	AddressImp::check( f ) ;
}

GNet::Address::Address( const AddressStorage & storage ) :
	m_4imp(std::make_unique<Address4>(storage.p(),storage.n()))
{
}

GNet::Address::Address( const sockaddr * addr , socklen_t len ) :
	m_4imp(std::make_unique<Address4>(addr,len))
{
}

GNet::Address::Address( const sockaddr * addr , socklen_t len , bool ) :
	m_4imp(std::make_unique<Address4>(addr,len))
{
}

GNet::Address::Address( const std::string & s ) :
	m_4imp(std::make_unique<Address4>(s))
{
}

GNet::Address::Address( const std::string & s , unsigned int port ) :
	m_4imp(std::make_unique<Address4>(s,port))
{
}

GNet::Address::Address( Family f , unsigned int port , int loopback_overload ) :
	m_4imp(std::make_unique<Address4>(port,loopback_overload))
{
	AddressImp::check( f ) ;
}

GNet::Address::Address( const Address & other ) :
	m_4imp(std::make_unique<Address4>(*other.m_4imp))
{
}

GNet::Address::Address( Address && other ) noexcept
=default ;

GNet::Address::~Address()
= default;

void GNet::Address::swap( Address & other ) noexcept
{
	using std::swap ;
	swap( m_4imp , other.m_4imp ) ;
}

GNet::Address & GNet::Address::operator=( const Address & other )
{
	Address(other).swap( *this ) ;
	return *this ;
}

GNet::Address & GNet::Address::operator=( Address && other ) noexcept
= default ;

GNet::Address GNet::Address::loopback( Family f , unsigned int port )
{
	return Address( f , port , 1 ) ;
}

GNet::Address & GNet::Address::setPort( unsigned int port )
{
	m_4imp->setPort( port ) ;
	return *this ;
}

GNet::Address & GNet::Address::setScopeId( unsigned long )
{
	return *this ; // not relevant for ipv4
}

bool GNet::Address::setZone( const std::string & )
{
	return true ; // not relevant for ipv4
}

unsigned int GNet::Address::bits() const
{
	return m_4imp->bits() ;
}

bool GNet::Address::isLoopback() const
{
	return m_4imp->isLoopback() ;
}

bool GNet::Address::isLocal( std::string & reason ) const
{
	return m_4imp->isLocal( reason ) ;
}

bool GNet::Address::isLinkLocal() const
{
	return m_4imp->isLinkLocal() ;
}

bool GNet::Address::isUniqueLocal() const
{
	return m_4imp->isUniqueLocal() ;
}

bool GNet::Address::isAny() const
{
	return m_4imp->isAny() ;
}

bool GNet::Address::is4() const
{
	return true ;
}

bool GNet::Address::is6() const
{
	return false ;
}

bool GNet::Address::same( const Address & other , bool ) const
{
	return m_4imp->same( *other.m_4imp ) ;
}

bool GNet::Address::operator==( const Address & other ) const
{
	return m_4imp->same( *other.m_4imp ) ;
}

bool GNet::Address::operator!=( const Address & other ) const
{
	return !( *this == other ) ;
}

bool GNet::Address::sameHostPart( const Address & other ) const
{
	return m_4imp->sameHostPart(*other.m_4imp) ;
}

std::string GNet::Address::displayString( bool ) const
{
	return m_4imp->displayString() ;
}

std::string GNet::Address::hostPartString() const
{
	return m_4imp->hostPartString() ;
}

std::string GNet::Address::queryString() const
{
	return m_4imp->queryString() ;
}

bool GNet::Address::validString( const std::string & s , std::string * reason_p )
{
	return Address4::validString( s , reason_p ) ;
}

bool GNet::Address::validStrings( const std::string & s1 , const std::string & s2 , std::string * reason_p )
{
	return Address4::validStrings( s1 , s2 , reason_p ) ;
}

sockaddr * GNet::Address::address()
{
	return m_4imp->address() ;
}

const sockaddr * GNet::Address::address() const
{
	return m_4imp->address() ;
}

socklen_t GNet::Address::length() const
{
	return Address4::length() ;
}

unsigned int GNet::Address::port() const
{
	return m_4imp->port() ;
}

unsigned long GNet::Address::scopeId( unsigned long default_ ) const
{
	return default_ ;
}

bool GNet::Address::validPort( unsigned int port )
{
	return Address4::validPort( port ) ;
}

bool GNet::Address::validData( const sockaddr * addr , socklen_t len )
{
	return Address4::validData( addr , len ) ;
}

int GNet::Address::domain() const
{
	return Address4::domain() ;
}

GNet::Address::Family GNet::Address::family() const
{
	return Family::ipv4 ;
}

int GNet::Address::af() const
{
	return AF_INET ;
}

G::StringArray GNet::Address::wildcards() const
{
	return m_4imp->wildcards() ;
}

// ==

//| \class GNet::AddressStorageImp
/// A pimple-pattern implementation class used by GNet::AddressStorage.
///
class GNet::AddressStorageImp
{
public:
	Address4::storage_type u ;
	socklen_t n ;
} ;

// ==

GNet::AddressStorage::AddressStorage() :
	m_imp(std::make_unique<AddressStorageImp>())
{
	static_assert( sizeof(Address4::sockaddr_type) <= sizeof(Address4::storage_type) , "" ) ;
	static_assert( alignof(Address4::sockaddr_type) <= alignof(Address4::storage_type) , "" ) ;
	m_imp->n = sizeof( Address4::storage_type ) ;
}

GNet::AddressStorage::~AddressStorage()
= default;

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
// fallback implementation for inet_pton() -- see gdef.h
int GNet::inet_pton_imp( int f , const char * p , void * result )
{
	if( p == nullptr || result == nullptr )
	{
		return 0 ; // just in case
	}
	else if( f == AF_INET )
	{
		sockaddr_in sa {} ;
		sa.sin_family = AF_INET ;
		sa.sin_addr.s_addr = inet_addr( p ) ;
		std::memcpy( result , &sa.sin_addr , sizeof(sa.sin_addr) ) ;
		return 1 ;
	}
	else
	{
		return -1 ;
	}
}
#endif

#if ! GCONFIG_HAVE_INET_NTOP
// fallback implementation for inet_ntop() -- see gdef.h
const char * GNet::inet_ntop_imp( int f , void * ap , char * buffer , std::size_t n )
{
	if( f == AF_INET )
	{
		std::ostringstream ss ;
		struct in_addr a ;
		std::memcpy( &a , ap , sizeof(a) ) ;
		ss << inet_ntoa( a ) ; // ignore warnings - this is not used if inet_ntop is available
		if( n <= ss.str().length() ) return nullptr ;
		std::strncpy( buffer , ss.str().c_str() , n ) ;
		return buffer ;
	}
	else
	{
		return nullptr ;
	}
}
#endif

