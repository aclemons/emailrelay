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
/// \file gaddress6.cpp
///

#include "gdef.h"
#include "gaddress6.h"
#include "gstr.h"
#include "gtest.h"
#include "gprocess.h"
#include "glog.h"
#include <algorithm> // std::swap()
#include <utility> // std::swap()
#include <cstring> // std::memcpy()
#include <climits>
#include <sys/types.h>
#include <sstream>
#include <vector>
#include <iomanip>
#include <array>

namespace GNet
{
	namespace Address6Imp
	{
		const char * port_separators = ":." ;
		char port_separator = '.' ;
	}
}

unsigned short GNet::Address6::af() noexcept
{
	return AF_INET6 ;
}

int GNet::Address6::domain() noexcept
{
	return PF_INET6 ;
}

GNet::Address6::Address6( std::nullptr_t ) :
	m_inet{}
{
	m_inet.sin6_family = af() ;
	m_inet.sin6_port = 0 ;
	m_inet.sin6_flowinfo = 0 ;
	gdef_address6_init( m_inet ) ; // gdef.h
}

GNet::Address6::Address6( unsigned int port ) :
	Address6(nullptr)
{
	m_inet.sin6_addr = in6addr_any ;
	const char * reason = setPort( m_inet , port ) ;
	if( reason ) throw Address::Error(reason) ;
}

GNet::Address6::Address6( unsigned int port , int /*loopback_overload*/ ) :
	Address6(nullptr)
{
	m_inet.sin6_addr = in6addr_loopback ;
	const char * reason = setPort( m_inet , port ) ;
	if( reason ) throw Address::Error(reason) ;
}

GNet::Address6::Address6( const sockaddr * addr , socklen_t len , bool ipv6_scope_id_fixup ) :
	Address6(nullptr)
{
	if( addr == nullptr )
		throw Address::Error() ;
	if( addr->sa_family != af() || static_cast<std::size_t>(len) < sizeof(sockaddr_type) )
		throw Address::BadFamily() ;

	std::memcpy( &m_inet , addr , sizeof(m_inet) ) ;

	if( ipv6_scope_id_fixup ) // for eg. NetBSD v7 getifaddrs() -- see FreeBSD Handbook "Scope Index"
	{
		auto hi = static_cast<unsigned int>( m_inet.sin6_addr.s6_addr[2] ) ;
		auto lo = static_cast<unsigned int>( m_inet.sin6_addr.s6_addr[3] ) ;
		m_inet.sin6_addr.s6_addr[2] = 0 ;
		m_inet.sin6_addr.s6_addr[3] = 0 ;
		m_inet.sin6_scope_id = ( hi << 8U | lo ) ;
	}
}

GNet::Address6::Address6( const std::string & host_part , unsigned int port ) :
	Address6(nullptr)
{
	const char * reason = setHostAddress( m_inet , host_part ) ;
	if( !reason )
		reason = setPort( m_inet , port ) ;
	if( reason )
		throw Address::BadString( std::string(reason) + ": " + host_part ) ;
}

GNet::Address6::Address6( const std::string & host_part , const std::string & port_part ) :
	Address6(nullptr)
{
	const char * reason = setHostAddress( m_inet , host_part ) ;
	if( !reason )
		reason = setPort( m_inet , port_part ) ;
	if( reason )
		throw Address::BadString( std::string(reason) + ": [" + host_part + "][" + port_part + "]" ) ;
}

GNet::Address6::Address6( const std::string & display_string ) :
	Address6(nullptr)
{
	const char * reason = setAddress( m_inet , display_string ) ;
	if( reason )
		throw Address::BadString( std::string(reason) + ": " + display_string ) ;
}

const char * GNet::Address6::setAddress( sockaddr_type & inet , const std::string & display_string )
{
	const std::string::size_type pos = display_string.find_last_of( Address6Imp::port_separators ) ;
	if( pos == std::string::npos )
		return "no port separator" ;

	std::string host_part = G::Str::head( display_string , pos ) ;
	std::string port_part = G::Str::tail( display_string , pos ) ;

	const char * reason = setHostAddress( inet , host_part ) ;
	if( !reason )
		reason = setPort( inet , port_part ) ;
	return reason ;
}

const char * GNet::Address6::setHostAddress( sockaddr_type & inet , const std::string & host_part )
{
	// wikipedia: "because all link-local addresses in a host have a common prefix, normal routing
	// procedures cannot be used to choose the outgoing interface when sending packets
	// to a link-local destination -- a special identifier, known as a zone index, is needed
	// to provide the additional routing information -- in the case of link-local addresses
	// zone indices correspond to interface identifiers -- when an address is written
	// textually the zone index is appended to the address separated by a percent sign --
	// the actual syntax of zone indices depends on the operating system"
	//
	// see also RFC-2553 section 4
	//
	std::string zone = G::Str::tail( host_part , host_part.find('%') , std::string() ) ;
	std::string host_part_head = G::Str::head( host_part , host_part.find('%') , host_part ) ;

	int rc = inet_pton( af() , host_part_head.c_str() , &inet.sin6_addr ) ;

	if( rc == 1 && !zone.empty() )
	{
		if( !setZone( inet , zone ) )
			return "invalid address zone/scope" ;
	}

	return rc == 1 ? nullptr : "invalid network address" ;
}

void GNet::Address6::setPort( unsigned int port )
{
	const char * reason = setPort( m_inet , port ) ;
	if( reason )
		throw Address::Error( "invalid port number" ) ;
}

const char * GNet::Address6::setPort( sockaddr_type & inet , const std::string & port_part )
{
	if( port_part.length() == 0U ) return "empty port string" ;
	if( !G::Str::isNumeric(port_part) || !G::Str::isUInt(port_part) ) return "non-numeric port string" ;
	return setPort( inet , G::Str::toUInt(port_part) ) ;
}

const char * GNet::Address6::setPort( sockaddr_type & inet , unsigned int port )
{
	if( port > 0xFFFFU ) return "port number too big" ;
	const g_port_t in_port = static_cast<g_port_t>(port) ;
	inet.sin6_port = htons( in_port ) ;
	return nullptr ;
}

bool GNet::Address6::setZone( const std::string & zone )
{
	return setZone( m_inet , zone ) ;
}

bool GNet::Address6::setZone( sockaddr_type & inet , const std::string & zone )
{
	unsigned long scope_id = 0UL ; // uint on unix, ULONG on windows
	if( G::Str::isULong(zone) )
	{
		scope_id = G::Str::toULong( zone ) ;
	}
	else
	{
		scope_id = gdef_if_nametoindex( zone.c_str() ) ; // see gdef.h
		if( scope_id == 0U )
			return false ;
	}
	inet.sin6_scope_id = scope_id ; // narrowing conversion on unix
	const bool no_overflow = scope_id == inet.sin6_scope_id ;
	return no_overflow ;
}

void GNet::Address6::setScopeId( unsigned long ipv6_scope_id )
{
	m_inet.sin6_scope_id = ipv6_scope_id ; // narrowing conversion on unix
}

std::string GNet::Address6::displayString( bool ipv6_with_scope_id ) const
{
	std::ostringstream ss ;
	ss << hostPartString() ;
	if( ipv6_with_scope_id && scopeId() != 0U )
		ss << "%" << scopeId() ;
	ss << Address6Imp::port_separator << port() ;
	return ss.str() ;
}

std::string GNet::Address6::hostPartString( bool /*raw*/ ) const
{
	std::array<char,INET6_ADDRSTRLEN+1U> buffer {} ;
	const void * vp = & m_inet.sin6_addr ;
	const char * p = inet_ntop( af() , const_cast<void*>(vp) , &buffer[0] , buffer.size() ) ; // cast for win32
	if( p == nullptr )
		throw Address::Error( "inet_ntop() failure" ) ;
	buffer[buffer.size()-1U] = '\0' ;
	return { &buffer[0] } ; // sic
}

std::string GNet::Address6::queryString() const
{
	std::ostringstream ss ;
	const char * hexmap = "0123456789abcdef" ;
	for( std::size_t i = 0U ; i < 16U ; i++ )
	{
		unsigned int n = static_cast<unsigned int>(m_inet.sin6_addr.s6_addr[15U-i]) % 256U ;
		ss << (i==0U?"":".") << hexmap[(n&15U)%16U] << "." << hexmap[(n>>4U)%16U] ;
	}
	return ss.str() ;
}

bool GNet::Address6::validData( const sockaddr * addr , socklen_t len )
{
	return addr != nullptr && addr->sa_family == af() && len == sizeof(sockaddr_type) ;
}

bool GNet::Address6::validString( const std::string & s , std::string * reason_p )
{
	sockaddr_type inet {} ;
	const char * reason = setAddress( inet , s ) ;
	if( reason && reason_p )
		*reason_p = std::string(reason) ;
	return reason == nullptr ;
}

bool GNet::Address6::validStrings( const std::string & host_part , const std::string & port_part ,
	std::string * reason_p )
{
	sockaddr_type inet {} ;
	const char * reason = setHostAddress( inet , host_part ) ;
	if( !reason )
		reason = setPort( inet , port_part ) ;
	if( reason && reason_p )
		*reason_p = std::string(reason) ;
	return reason == nullptr ;
}

bool GNet::Address6::validPort( unsigned int port )
{
	sockaddr_type inet {} ;
	const char * reason = setPort( inet , port ) ;
	return reason == nullptr ;
}

bool GNet::Address6::same( const Address6 & other , bool with_scope ) const
{
	return
		m_inet.sin6_family == af() &&
		other.m_inet.sin6_family == af() &&
		sameAddr( m_inet.sin6_addr , other.m_inet.sin6_addr ) &&
		( !with_scope || m_inet.sin6_scope_id == other.m_inet.sin6_scope_id ) &&
		m_inet.sin6_port == other.m_inet.sin6_port ;
}

bool GNet::Address6::sameHostPart( const Address6 & other , bool with_scope ) const
{
	return
		m_inet.sin6_family == af() &&
		other.m_inet.sin6_family == af() &&
		sameAddr( m_inet.sin6_addr , other.m_inet.sin6_addr ) &&
		( !with_scope || m_inet.sin6_scope_id == other.m_inet.sin6_scope_id ) ;
}

bool GNet::Address6::sameAddr( const ::in6_addr & a , const ::in6_addr & b )
{
	for( std::size_t i = 0 ; i < 16U ; i++ )
	{
		if( a.s6_addr[i] != b.s6_addr[i] )
			return false ;
	}
	return true ;
}

unsigned int GNet::Address6::port() const
{
	return ntohs( m_inet.sin6_port ) ;
}

unsigned long GNet::Address6::scopeId( unsigned long /*default*/ ) const
{
	return m_inet.sin6_scope_id ;
}

const sockaddr * GNet::Address6::address() const
{
	// core guidelines: C.183
	// type-punning allowed by "common initial sequence" rule
	return reinterpret_cast<const sockaddr*>( &m_inet ) ;
}

sockaddr * GNet::Address6::address()
{
	return reinterpret_cast<sockaddr*>( &m_inet ) ;
}

socklen_t GNet::Address6::length() noexcept
{
	return sizeof(sockaddr_type) ;
}

namespace GNet
{
	namespace Address6Imp
	{
		bool shiftLeft( struct in6_addr & mask )
		{
			bool carry_out = false ;
			bool carry_in = false ;
			for( int i = 15 ; i >= 0 ; i-- )
			{
				const unsigned char top_bit = 128U ;
				carry_out = !!( mask.s6_addr[i] & top_bit ) ;
				mask.s6_addr[i] <<= 1U ;
				if( carry_in ) ( mask.s6_addr[i] |= 1U ) ;
				carry_in = carry_out ;
			}
			return carry_out ;
		}
		void shiftLeft( struct in6_addr & mask , unsigned int bits )
		{
			for( unsigned int i = 0U ; i < bits ; i++ )
				shiftLeft( mask ) ;
		}
		void reset( struct in6_addr & addr )
		{
			for( unsigned int i = 0 ; i < 16U ; i++ )
				addr.s6_addr[i] = 0 ;
		}
		void fill( struct in6_addr & addr )
		{
			for( unsigned int i = 0 ; i < 16U ; i++ )
				addr.s6_addr[i] = 0xff ;
		}
		struct in6_addr make( unsigned int lhs_hi , unsigned int lhs_lo , unsigned int rhs )
		{
			struct in6_addr addr {} ;
			reset( addr ) ;
			addr.s6_addr[15] = rhs ;
			addr.s6_addr[0] = lhs_hi ;
			addr.s6_addr[1] = lhs_lo ;
			return addr ;
		}
		void applyMask( struct in6_addr & addr , const struct in6_addr & mask )
		{
			for( int i = 0 ; i < 16 ; i++ )
			{
				addr.s6_addr[i] &= mask.s6_addr[i] ;
			}
		}
		struct in6_addr mask( unsigned int bits )
		{
			struct in6_addr addr {} ;
			fill( addr ) ;
			shiftLeft( addr , 128U - bits ) ;
			return addr ;
		}
		struct in6_addr masked( const struct in6_addr & addr_in , const struct in6_addr & mask )
		{
			struct in6_addr result = addr_in ;
			applyMask( result , mask ) ;
			return result ;
		}
	}
}

G::StringArray GNet::Address6::wildcards() const
{
	namespace imp = Address6Imp ;
	Address6 a( *this ) ;

	G::StringArray result ;
	result.reserve( 128U ) ;
	result.push_back( hostPartString() ) ;

	struct in6_addr mask {} ;
	imp::fill( mask ) ;

	for( int bit = 0 ; bit < 128 ; bit++ )
	{
		std::ostringstream ss ;
		ss << a.hostPartString() << "/" << (128-bit) ;
		result.push_back( ss.str() ) ;

		imp::shiftLeft( mask ) ;
		imp::applyMask( a.m_inet.sin6_addr , mask ) ;
	}
	return result ;
}

unsigned int GNet::Address6::bits() const
{
	namespace imp = Address6Imp ;
	struct in6_addr a = m_inet.sin6_addr ;
	unsigned int count = 0U ;
	while( imp::shiftLeft(a) )
		count++ ;
	return count ;
}

bool GNet::Address6::isLocal( std::string & reason ) const
{
	if( isLoopback() || isLinkLocal() || isUniqueLocal() )
	{
		return true ;
	}
	else
	{
		std::ostringstream ss ;
		ss << hostPartString() << " is not in ::1/128 or fe80::/64 or fc00::/7" ;
		reason = ss.str() ;
		return false ;
	}
}

bool GNet::Address6::isLoopback() const
{
	// ::1/128 (cf. 127.0.0.0/8)
	namespace imp = Address6Imp ;
	struct in6_addr _1 = imp::make( 0U , 0U , 1U ) ; /// ::1/128
	return sameAddr( _1 , m_inet.sin6_addr ) ;
}

bool GNet::Address6::isLinkLocal() const
{
	// fe80::/64 (cf. 169.254.0.0/16)
	namespace imp = Address6Imp ;
	struct in6_addr addr_64 = imp::masked( m_inet.sin6_addr , imp::mask(64U) ) ;
	struct in6_addr _fe80 = imp::make( 0xfeU , 0x80U , 0U ) ;
	return sameAddr( _fe80 , addr_64 ) ;
}

bool GNet::Address6::isMulticast() const
{
	return false ; // TODO ipv6 multicast
}

bool GNet::Address6::isUniqueLocal() const
{
	// fc00::/7 (cf. 192.168.0.0/16 or 10.0.0.0/8)
	namespace imp = Address6Imp ;
	struct in6_addr addr_7 = imp::masked( m_inet.sin6_addr , imp::mask(7U) ) ;
	struct in6_addr _fc00 = imp::make( 0xfcU , 0U , 0U ) ;
	return sameAddr( _fc00 , addr_7 ) ;
}

bool GNet::Address6::isAny() const
{
	for( int i = 0 ; i < 16 ; i++ )
	{
		if( m_inet.sin6_addr.s6_addr[i] != in6addr_any.s6_addr[i] )
			return false ;
	}
	return true ;
}

