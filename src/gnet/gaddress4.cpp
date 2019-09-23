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
//
// gaddress4.cpp
//
// This file is formatted for side-by-side comparison with gaddress6.cpp.

#include "gdef.h"
#include "gaddress4.h"
#include "gstr.h"
#include "gassert.h"
#include "glog.h"
#include <algorithm> // std::swap()
#include <utility> // std::swap()
#include <climits>
#include <sys/types.h>
#include <sstream>
//
//

namespace
{
	const char * port_separators = ":/" ;
	char port_separator = ':' ;
}

unsigned short GNet::Address4::family()
{
	return AF_INET ;
}

int GNet::Address4::domain()
{
	return PF_INET ;
}

void GNet::Address4::init()
{
	static specific_type zero ;
	m_inet.specific = zero ;
	m_inet.specific.sin_family =  family() ;
	m_inet.specific.sin_port =  0 ;
//
//
}

GNet::Address4::Address4( unsigned int port )
{
	init() ;
	m_inet.specific.sin_addr.s_addr = htonl(INADDR_ANY);
	const char * reason = setPort( m_inet , port ) ;
	if( reason ) throw Address::Error(reason) ;
}

GNet::Address4::Address4( unsigned int port , int )
{
	init() ;
	m_inet.specific.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	const char * reason = setPort( m_inet , port ) ;
	if( reason ) throw Address::Error(reason) ;
}

GNet::Address4::Address4( const sockaddr * addr , socklen_t len )
{
	init() ;
	if( addr == nullptr )
		throw Address::Error() ;
	if( addr->sa_family != family() || static_cast<size_t>(len) < sizeof(specific_type) )
		throw Address::BadFamily() ;

	m_inet.specific = *(reinterpret_cast<const specific_type*>(addr)) ;
}

GNet::Address4::Address4( const Address4 & other )
{
	m_inet.specific = other.m_inet.specific ;
}

GNet::Address4::Address4( const std::string & host_part , unsigned int port )
{
	init() ;
	const char * reason = setHostAddress( m_inet , host_part ) ;
	if( !reason )
		reason = setPort( m_inet , port ) ;
	if( reason )
		throw Address::BadString( std::string(reason) + ": " + host_part ) ;
}

GNet::Address4::Address4( const std::string & host_part , const std::string & port_part )
{
	init() ;
	const char * reason = setHostAddress( m_inet , host_part ) ;
	if( !reason )
		reason = setPort( m_inet , port_part ) ;
	if( reason )
		throw Address::BadString( std::string(reason) + ": [" + host_part + "][" + port_part + "]" ) ;
}

GNet::Address4::Address4( const std::string & display_string )
{
	init() ;
	const char * reason = setAddress( m_inet , display_string ) ;
	if( reason )
		throw Address::BadString( std::string(reason) + ": " + display_string ) ;
}

const char * GNet::Address4::setAddress( union_type & inet , const std::string & display_string )
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

const char * GNet::Address4::setHostAddress( union_type & inet , const std::string & host_part )
{
	// start with a stricter check than inet_pton(), inet_addr() etc. since they allow eg. "123.123"
	if( !Address4::format(host_part) )
		return "invalid ipv4 network address format" ;

	int rc = inet_pton( family() , host_part.c_str() , &inet.specific.sin_addr ) ;
	return rc == 1 ? nullptr : "invalid ipv4 network address" ;
}

void GNet::Address4::setPort( unsigned int port )
{
	const char * reason = setPort( m_inet , port ) ;
	if( reason )
		throw Address::Error( "invalid port number" ) ;
}

const char * GNet::Address4::setPort( union_type & inet , const std::string & port_part )
{
	if( port_part.length() == 0U ) return "empty port string" ;
	if( !G::Str::isNumeric(port_part) || !G::Str::isUInt(port_part) ) return "non-numeric port string" ;
	return setPort( inet , G::Str::toUInt(port_part) ) ;
}

const char * GNet::Address4::setPort( union_type & inet , unsigned int port )
{
	if( port > 0xFFFFU ) return "port number too big" ;
	const g_port_t in_port = static_cast<g_port_t>(port) ;
	inet.specific.sin_port = htons( in_port ) ;
	return nullptr ;
}

std::string GNet::Address4::displayString() const
{
	std::ostringstream ss ;
	ss << hostPartString() ;
	ss << port_separator << port() ;
	return ss.str() ;
}

std::string GNet::Address4::hostPartString() const
{
	char buffer[INET_ADDRSTRLEN+1U] ;
	const void * vp = & m_inet.specific.sin_addr ;
	const char * p = inet_ntop( family() , const_cast<void*>(vp) , buffer , sizeof(buffer) ) ;
	if( p == nullptr )
		throw Address::Error( "inet_ntop() failure" ) ;
	buffer[sizeof(buffer)-1U] = '\0' ;
	return std::string(buffer) ;
}

std::string GNet::Address4::queryString() const
{
	G::StringArray parts = G::Str::splitIntoFields( hostPartString() , "." ) ;
	std::reverse( parts.begin() , parts.end() ) ;
	return G::Str::join( "." , parts ) ;
}






bool GNet::Address4::validData( const sockaddr * addr , socklen_t len )
{
	return addr != nullptr && addr->sa_family == family() && len == sizeof(specific_type) ;
}

bool GNet::Address4::validString( const std::string & s , std::string * reason_p )
{
	union_type inet ;
	const char * reason = setAddress( inet , s ) ;
	if( reason && reason_p )
		*reason_p = std::string(reason) ;
	return reason == nullptr ;
}

bool GNet::Address4::validStrings( const std::string & host_part , const std::string & port_part , std::string * reason_p )
{
	union_type inet ;
	const char * reason = setHostAddress( inet , host_part ) ;
	if( !reason )
		reason = setPort( inet , port_part ) ;
	if( reason && reason_p )
		*reason_p = std::string(reason) ;
	return reason == nullptr ;
}

bool GNet::Address4::validPort( unsigned int port )
{
	union_type inet ;
	const char * reason = setPort( inet , port ) ;
	return reason == nullptr ;
}

bool GNet::Address4::same( const Address4 & other ) const
{
	return
		m_inet.specific.sin_family == family() &&
		other.m_inet.specific.sin_family == family() &&
		sameAddr( m_inet.specific.sin_addr , other.m_inet.specific.sin_addr ) &&
		m_inet.specific.sin_port == other.m_inet.specific.sin_port ;
}

bool GNet::Address4::sameHostPart( const Address4 & other ) const
{
	return
		m_inet.specific.sin_family == family() &&
		other.m_inet.specific.sin_family == family() &&
		sameAddr( m_inet.specific.sin_addr , other.m_inet.specific.sin_addr ) ;
}

bool GNet::Address4::sameAddr( const ::in_addr & a , const ::in_addr & b )
{
	return a.s_addr == b.s_addr ;
}

unsigned int GNet::Address4::port() const
{
	return ntohs( m_inet.specific.sin_port ) ;
}

const sockaddr * GNet::Address4::address() const
{
	return & m_inet.general ;
}

sockaddr * GNet::Address4::address()
{
	return & m_inet.general ;
}

socklen_t GNet::Address4::length()
{
	return sizeof(specific_type) ;
}

G::StringArray GNet::Address4::wildcards() const
{
	std::string ip_string = hostPartString() ;

	G::StringArray result ;
	result.reserve( 38U ) ;
	result.push_back( ip_string ) ;

	G::StringArray part ;
	part.reserve( 4U ) ;
	G::Str::splitIntoFields( ip_string , part , "." ) ;

	G_ASSERT( part.size() == 4U ) ;
	if( part.size() != 4U ||
		part[0].empty() || !G::Str::isUInt(part[0]) ||
		part[1].empty() || !G::Str::isUInt(part[1]) ||
		part[2].empty() || !G::Str::isUInt(part[2]) ||
		part[3].empty() || !G::Str::isUInt(part[3]) )
	{
		return result ;
	}

	unsigned int n0 = G::Str::toUInt(part[0]) ;
	unsigned int n1 = G::Str::toUInt(part[1]) ;
	unsigned int n2 = G::Str::toUInt(part[2]) ;
	unsigned int n3 = G::Str::toUInt(part[3]) ;

	std::string part_0_1_2 = part[0] ;
	part_0_1_2.append( 1U , '.' ) ;
	part_0_1_2.append( part[1] ) ;
	part_0_1_2.append( 1U , '.' ) ;
	part_0_1_2.append( part[2] ) ;
	part_0_1_2.append( 1U , '.' ) ;

	std::string part_0_1 = part[0] ;
	part_0_1.append( 1U , '.' ) ;
	part_0_1.append( part[1] ) ;
	part_0_1.append( 1U , '.' ) ;

	std::string part_0 = part[0] ;
	part_0.append( 1U , '.' ) ;

	const std::string empty ;

	add( result , part_0_1_2 , n3 & 0xff , "/32" ) ;
	add( result , part_0_1_2 , n3 & 0xfe , "/31" ) ;
	add( result , part_0_1_2 , n3 & 0xfc , "/30" ) ;
	add( result , part_0_1_2 , n3 & 0xf8 , "/29" ) ;
	add( result , part_0_1_2 , n3 & 0xf0 , "/28" ) ;
	add( result , part_0_1_2 , n3 & 0xe0 , "/27" ) ;
	add( result , part_0_1_2 , n3 & 0xc0 , "/26" ) ;
	add( result , part_0_1_2 , n3 & 0x80 , "/25" ) ;
	add( result , part_0_1_2 , 0 , "/24" ) ;
	add( result , part_0_1_2 , "*" ) ;
	add( result , part_0_1 , n2 & 0xfe , ".0/23" ) ;
	add( result , part_0_1 , n2 & 0xfc , ".0/22" ) ;
	add( result , part_0_1 , n2 & 0xfc , ".0/21" ) ;
	add( result , part_0_1 , n2 & 0xf8 , ".0/20" ) ;
	add( result , part_0_1 , n2 & 0xf0 , ".0/19" ) ;
	add( result , part_0_1 , n2 & 0xe0 , ".0/18" ) ;
	add( result , part_0_1 , n2 & 0xc0 , ".0/17" ) ;
	add( result , part_0_1 , 0 , ".0/16" ) ;
	add( result , part_0_1 , "*.*" ) ;
	add( result , part_0 , n1 & 0xfe , ".0.0/15" ) ;
	add( result , part_0 , n1 & 0xfc , ".0.0/14" ) ;
	add( result , part_0 , n1 & 0xf8 , ".0.0/13" ) ;
	add( result , part_0 , n1 & 0xf0 , ".0.0/12" ) ;
	add( result , part_0 , n1 & 0xe0 , ".0.0/11" ) ;
	add( result , part_0 , n1 & 0xc0 , ".0.0/10" ) ;
	add( result , part_0 , n1 & 0x80 , ".0.0/9" ) ;
	add( result , part_0 , 0 , ".0.0/8" ) ;
	add( result , part_0 , "*.*.*" ) ;
	add( result , empty , n0 & 0xfe , ".0.0.0/7" ) ;
	add( result , empty , n0 & 0xfc , ".0.0.0/6" ) ;
	add( result , empty , n0 & 0xf8 , ".0.0.0/5" ) ;
	add( result , empty , n0 & 0xf0 , ".0.0.0/4" ) ;
	add( result , empty , n0 & 0xe0 , ".0.0.0/3" ) ;
	add( result , empty , n0 & 0xc0 , ".0.0.0/2" ) ;
	add( result , empty , n0 & 0x80 , ".0.0.0/1" ) ;
	add( result , empty , 0 , ".0.0.0/0" ) ;
	add( result , empty , "*.*.*.*" ) ;

	return result ;
}

void GNet::Address4::add( G::StringArray & result , const std::string & head , unsigned int n , const char * tail )
{
	std::string s = head ;
	s.append( G::Str::fromUInt( n ) ) ;
	s.append( tail ) ;
	result.push_back( s ) ;
}

void GNet::Address4::add( G::StringArray & result , const std::string & head , const char * tail )
{
	result.push_back( head + tail ) ;
}

bool GNet::Address4::format( std::string s )
{
	// an independent check for the IPv4 dotted-quad format

	if( s.empty() || s.find_first_not_of("0123456789.") != std::string::npos ||
		std::count(s.begin(),s.end(),'.') != 3U || s.at(0U) == '.' ||
		s.at(s.length()-1U) == '.' || s.find("..") != std::string::npos )
			return false ;

	unsigned int n = 0U ;
	for( std::string::iterator pp = s.begin() ; pp != s.end() ; ++pp )
	{
		n = (*pp) == '.' ? 0U : ( ( n * 10U ) + (static_cast<unsigned int>(*pp)-static_cast<unsigned int>('0')) ) ;
		if( n >= 256U )
			return false ;
	}
	return true ;
}

unsigned int GNet::Address4::bits() const
{
	const unsigned long a = ntohl( m_inet.specific.sin_addr.s_addr ) ;
	unsigned int count = 0U ;
	for( unsigned long mask = 0x80000000U ; mask && ( a & mask ) ; mask >>= 1 )
		count++ ;
	return count ;
}

bool GNet::Address4::isLoopback() const
{
	// 127.0.0.0/8
	return ( ntohl(m_inet.specific.sin_addr.s_addr) >> 24 ) == 127U ;
}

bool GNet::Address4::isLocal( std::string & reason ) const
{
	// this is a dummy implementation that only tests for loopback
	// addresses -- prefer GNet::Local::isLocal()
	//
	if( isLoopback() )
	{
		return true ;
	}
	else
	{
		std::ostringstream ss ;
		ss << hostPartString() << " is not a loopback address" ;
		reason = ss.str() ;
		return false ;
	}
}

bool GNet::Address4::isPrivate() const
{
	// RFC-1918
	return
		( ntohl(m_inet.specific.sin_addr.s_addr) >> 24 ) == 0x0A || // 10.0.0.0/8
		( ntohl(m_inet.specific.sin_addr.s_addr) >> 20 ) == 0xAC1 || // 172.16.0.0/12
		( ntohl(m_inet.specific.sin_addr.s_addr) >> 16 ) == 0xC0A8 ; // 192.168.0.0/16
}

/// \file gaddress4.cpp
