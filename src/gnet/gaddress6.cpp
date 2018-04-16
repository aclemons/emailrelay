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
// gaddress6.cpp
//
// This file is formatted for side-by-side comparison with gaddress4.cpp.

#include "gdef.h"
#include "gaddress6.h"
#include "gstrings.h"
#include "gstr.h"
#include "gassert.h"
#include "gdebug.h"
#include <utility> // std::swap()
#include <algorithm> // std::swap()
#include <climits>
#include <sys/types.h>
#include <sstream>
#include <vector>
#include <iomanip>

namespace
{
	const char * port_separators = ":/." ;
	char port_separator = '.' ;
}

unsigned short GNet::Address6::family()
{
	return AF_INET6 ;
}

int GNet::Address6::domain()
{
	return PF_INET6 ;
}

void GNet::Address6::init()
{
	static specific_type zero ;
	m_inet.specific = zero ;
	m_inet.specific.sin6_family = family() ;
	m_inet.specific.sin6_flowinfo = 0 ;
	m_inet.specific.sin6_port = 0 ;
	gnet_address6_init( m_inet.specific ) ; // gdef.h
}

GNet::Address6::Address6( unsigned int port )
{
	init() ;
	m_inet.specific.sin6_addr = in6addr_any ;
	const char * reason = setPort( m_inet , port ) ;
	if( reason ) throw Address::Error(reason) ;
}

GNet::Address6::Address6( unsigned int port , int )
{
	init() ;
	m_inet.specific.sin6_addr = in6addr_loopback ;
	const char * reason = setPort( m_inet , port ) ;
	if( reason ) throw Address::Error(reason) ;
}

GNet::Address6::Address6( const sockaddr * addr , socklen_t len )
{
	init() ;
	if( addr == nullptr )
		throw Address::Error() ;
	if( addr->sa_family != family() || static_cast<size_t>(len) < sizeof(specific_type) )
		throw Address::BadFamily() ;

	m_inet.specific = *(reinterpret_cast<const specific_type*>(addr)) ;
}

GNet::Address6::Address6( const Address6 & other )
{
	m_inet.specific = other.m_inet.specific ;
}

GNet::Address6::Address6( const std::string & host_part , unsigned int port )
{
	init() ;
	const char * reason = setHostAddress( m_inet , host_part ) ;
	if( !reason )
		reason = setPort( m_inet , port ) ;
	if( reason )
		throw Address::BadString( std::string(reason) + ": " + host_part ) ;
}

GNet::Address6::Address6( const std::string & host_part , const std::string & port_part )
{
	init() ;
	const char * reason = setHostAddress( m_inet , host_part ) ;
	if( !reason )
		reason = setPort( m_inet , port_part ) ;
	if( reason )
		throw Address::BadString( std::string(reason) + ": [" + host_part + "][" + port_part + "]" ) ;
}

GNet::Address6::Address6( const std::string & display_string )
{
	init() ;
	const char * reason = setAddress( m_inet , display_string ) ;
	if( reason )
		throw Address::BadString( std::string(reason) + ": " + display_string ) ;
}

const char * GNet::Address6::setAddress( union_type & inet , const std::string & display_string )
{
	const std::string::size_type pos = display_string.find_last_of( port_separators ) ;
	if( pos == std::string::npos )
		return "no port separator" ;

	std::string host_part = G::Str::head( display_string , pos ) ;
	std::string port_part = G::Str::tail( display_string , pos ) ;

	const char * reason = setHostAddress( inet , host_part ) ;
	if( !reason )
		reason = setPort( inet , port_part ) ;
	return reason ;
}

const char * GNet::Address6::setHostAddress( union_type & inet , const std::string & host_part )
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

	int rc = inet_pton( family() , host_part_head.c_str() , &inet.specific.sin6_addr ) ;

	if( rc == 1 && !zone.empty() )
	{
		unsigned int zone_id = G::Str::toUInt( zone , "0" ) ;
		if( zone_id == 0U )
			zone_id = if_nametoindex( zone.c_str() ) ; // autoconf test -- see gdef.h
		if( zone_id != 0U )
			setZone( inet , zone_id ) ; // sin6_scope_id
	}

	return rc == 1 ? nullptr : "invalid network address" ;
}

void GNet::Address6::setPort( unsigned int port )
{
	const char * reason = setPort( m_inet , port ) ;
	if( reason )
		throw Address::Error( "invalid port number" ) ;
}

const char * GNet::Address6::setPort( union_type & inet , const std::string & port_part )
{
	if( port_part.length() == 0U ) return "empty port string" ;
	if( !G::Str::isNumeric(port_part) || !G::Str::isUInt(port_part) ) return "non-numeric port string" ;
	return setPort( inet , G::Str::toUInt(port_part) ) ;
}

const char * GNet::Address6::setPort( union_type & inet , unsigned int port )
{
	if( port > 0xFFFFU ) return "port number too big" ;
	const g_port_t in_port = static_cast<g_port_t>(port) ;
	inet.specific.sin6_port = htons( in_port ) ;
	return nullptr ;
}

void GNet::Address6::setZone( union_type & inet , unsigned int zone_id )
{
	inet.specific.sin6_scope_id = zone_id ;
	//inet.specific.sin6_scope_struct.Level = 0 ;
	//inet.specific.sin6_scope_struct.Zone = zone_id ;
}

std::string GNet::Address6::displayString() const
{
	const bool with_scope_id = false ;
	std::ostringstream ss ;
	ss << hostPartString() ;
	if( with_scope_id )
		ss << "%" << scopeId() ;
	ss << port_separator << port() ;
	return ss.str() ;
}

std::string GNet::Address6::hostPartString() const
{
	char buffer[INET6_ADDRSTRLEN+1U] ;
	const void * vp = & m_inet.specific.sin6_addr ;
	const char * p = inet_ntop( family() , const_cast<void*>(vp) , buffer , sizeof(buffer) ) ; // (const cast for windows)
	if( p == nullptr )
		throw Address::Error( "inet_ntop() failure" ) ;
	buffer[sizeof(buffer)-1U] = '\0' ;
	return std::string(buffer) ;
}

bool GNet::Address6::validData( const sockaddr * addr , socklen_t len )
{
	return addr != nullptr && addr->sa_family == family() && len == sizeof(specific_type) ;
}

bool GNet::Address6::validString( const std::string & s , std::string * reason_p )
{
	union_type inet ;
	const char * reason = setAddress( inet , s ) ;
	if( reason && reason_p )
		*reason_p = std::string(reason) ;
	return reason == nullptr ;
}

bool GNet::Address6::validStrings( const std::string & host_part , const std::string & port_part , std::string * reason_p )
{
	union_type inet ;
	const char * reason = setHostAddress( inet , host_part ) ;
	if( !reason )
		reason = setPort( inet , port_part ) ;
	if( reason && reason_p )
		*reason_p = std::string(reason) ;
	return reason == nullptr ;
}

bool GNet::Address6::validPort( unsigned int port )
{
	union_type inet ;
	const char * reason = setPort( inet , port ) ;
	return reason == nullptr ;
}

bool GNet::Address6::same( const Address6 & other ) const
{
	return
		m_inet.specific.sin6_family == family() &&
		other.m_inet.specific.sin6_family == family() &&
		sameAddr( m_inet.specific.sin6_addr , other.m_inet.specific.sin6_addr ) &&
		m_inet.specific.sin6_port == other.m_inet.specific.sin6_port ;
}

bool GNet::Address6::sameHostPart( const Address6 & other ) const
{
	return
		m_inet.specific.sin6_family == family() &&
		other.m_inet.specific.sin6_family == family() &&
		sameAddr( m_inet.specific.sin6_addr , other.m_inet.specific.sin6_addr ) ;
}

bool GNet::Address6::sameAddr( const ::in6_addr & a , const ::in6_addr & b )
{
	for( size_t i = 0 ; i < 16U ; i++ )
	{
		if( a.s6_addr[i] != b.s6_addr[i] )
			return false ;
	}
	return true ;
}

unsigned int GNet::Address6::port() const
{
	return ntohs( m_inet.specific.sin6_port ) ;
}

unsigned long GNet::Address6::scopeId() const
{
	return m_inet.specific.sin6_scope_id ;
}

const sockaddr * GNet::Address6::address() const
{
	return &m_inet.general ;
}

sockaddr * GNet::Address6::address()
{
	return &m_inet.general ;
}

socklen_t GNet::Address6::length()
{
	return sizeof(specific_type) ;
}

namespace
{
	void shiftLeft( struct in6_addr & mask )
	{
		bool carry_in = false ;
		for( int i = 15 ; i >= 0 ; i-- )
		{
			const unsigned char top_bit = 128U ;
			bool carry_out = !!( mask.s6_addr[i] & top_bit ) ;
			mask.s6_addr[i] <<= 1U ;
			if( carry_in ) ( mask.s6_addr[i] |= 1U ) ;
			carry_in = carry_out ;
		}
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
		struct in6_addr addr ;
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
		struct in6_addr addr ;
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

G::StringArray GNet::Address6::wildcards() const
{
	Address6 a( *this ) ;

	G::StringArray result ;
	result.reserve( 128U ) ;
	result.push_back( hostPartString() ) ;

	struct in6_addr mask ;
	fill( mask ) ;

	for( int bit = 0 ; bit < 128 ; bit++ )
	{
		std::ostringstream ss ;
		ss << a.hostPartString() << "/" << (128-bit) ;
		result.push_back( ss.str() ) ;

		shiftLeft( mask ) ;
		applyMask( a.m_inet.specific.sin6_addr , mask ) ;
	}
	return result ;
}

bool GNet::Address6::isLoopback() const
{
	// ::1/128
	struct in6_addr _1 = make( 0U , 0U , 1U ) ;
	return sameAddr( _1 , m_inet.specific.sin6_addr ) ;
}

bool GNet::Address6::isLocal( std::string & reason ) const
{
	struct in6_addr addr_128 = masked( m_inet.specific.sin6_addr , mask(128U) ) ; // degenerate mask
	struct in6_addr addr_64 = masked( m_inet.specific.sin6_addr , mask(64U) ) ;
	struct in6_addr addr_7 = masked( m_inet.specific.sin6_addr , mask(7U) ) ;

	struct in6_addr _1 = make( 0U , 0U , 1U ) ;
	struct in6_addr _fe80 = make( 0xfeU , 0x80U , 0U ) ;
	struct in6_addr _fc00 = make( 0xfcU , 0U , 0U ) ;

	bool local =
		sameAddr( _1 , addr_128 ) ||
		sameAddr( _fe80 , addr_64 ) ||
		sameAddr( _fc00 , addr_7 ) ;

	if( !local )
	{
		std::ostringstream ss ;
		ss << hostPartString() << " is not ::1/128 or in fe80::/64 or fc00::/7" ;
		reason = ss.str() ;
	}
	return local ;
}

/// \file gaddress6.cpp
