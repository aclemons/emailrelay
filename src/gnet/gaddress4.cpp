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
/// \file gaddress4.cpp
///

#include "gdef.h"
#include "gaddress4.h"
#include "gstr.h"
#include "gstringview.h"
#include "gstringfield.h"
#include "gtest.h"
#include "gassert.h"
#include "glog.h"
#include <algorithm> // std::swap()
#include <utility> // std::swap()
#include <climits>
#include <sys/types.h>
#include <sstream>
#include <array>

namespace GNet
{
	namespace Address4Imp
	{
		constexpr std::string_view port_separators {":",1U} ;
		constexpr char port_separator = ':' ;
	}
}

unsigned short GNet::Address4::af() noexcept
{
	return AF_INET ;
}

int GNet::Address4::domain() noexcept
{
	return PF_INET ;
}

GNet::Address4::Address4( std::nullptr_t ) :
	m_inet{}
{
	m_inet.sin_family =  af() ;
	m_inet.sin_port =  0 ;
}

GNet::Address4::Address4( unsigned int port ) :
	Address4(nullptr)
{
	m_inet.sin_addr.s_addr = htonl(INADDR_ANY);
	const char * reason = setPort( m_inet , port ) ;
	if( reason ) throw Address::Error(reason) ;
}

GNet::Address4::Address4( unsigned int port , int /*loopback_overload*/ ) :
	Address4(nullptr)
{
	m_inet.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	const char * reason = setPort( m_inet , port ) ;
	if( reason ) throw Address::Error(reason) ;
}

GNet::Address4::Address4( const sockaddr * addr , socklen_t len ) :
	Address4(nullptr)
{
	if( addr == nullptr )
		throw Address::Error() ;
	if( addr->sa_family != af() || static_cast<std::size_t>(len) < sizeof(sockaddr_type) )
		throw Address::BadFamily() ;

	m_inet = *(reinterpret_cast<const sockaddr_type*>(addr)) ;
}

GNet::Address4::Address4( std::string_view host_part , std::string_view port_part ) :
	Address4(nullptr)
{
	const char * reason = setHostAddress( m_inet , host_part ) ;
	if( !reason )
		reason = setPort( m_inet , port_part ) ;
	if( reason )
		throw Address::BadString( std::string(reason).append(": [").append(host_part.data(),host_part.size()).append("][").append(port_part.data(),port_part.size()).append(1U,']') ) ;
}

GNet::Address4::Address4( std::string_view display_string ) :
	Address4(nullptr)
{
	const char * reason = setAddress( m_inet , display_string ) ;
	if( reason )
		throw Address::BadString( std::string(reason).append(": ").append(display_string.data(),display_string.size()) ) ;
}

const char * GNet::Address4::setAddress( sockaddr_type & inet , std::string_view display_string )
{
	const std::string::size_type pos = display_string.find_last_of( Address4Imp::port_separators ) ;
	if( pos == std::string::npos )
		return "no port separator" ;

	std::string_view host_part = G::Str::headView( display_string , pos ) ;
	std::string_view port_part = G::Str::tailView( display_string , pos ) ;

	const char * reason = setHostAddress( inet , host_part ) ;
	if( !reason )
		reason = setPort( inet , port_part ) ;
	return reason ;
}

const char * GNet::Address4::setHostAddress( sockaddr_type & inet , std::string_view host_part )
{
	// start with a stricter check than inet_pton(), inet_addr() etc. since they allow eg. "123.123"
	if( !Address4::format(host_part) )
		return "invalid network address" ;

	int rc = inet_pton( af() , G::sv_to_string(host_part).c_str() , &inet.sin_addr ) ;
	return rc == 1 ? nullptr : "invalid network address" ;
}

void GNet::Address4::setPort( unsigned int port )
{
	const char * reason = setPort( m_inet , port ) ;
	if( reason )
		throw Address::Error( "invalid port number" ) ;
}

const char * GNet::Address4::setPort( sockaddr_type & inet , std::string_view port_part )
{
	if( port_part.empty() ) return "empty port string" ;
	if( !G::Str::isNumeric(port_part) || !G::Str::isUInt(port_part) ) return "non-numeric port string" ;
	return setPort( inet , G::Str::toUInt(port_part) ) ;
}

const char * GNet::Address4::setPort( sockaddr_type & inet , unsigned int port )
{
	if( port > 0xFFFFU ) return "port number too big" ;
	const g_port_t in_port = static_cast<g_port_t>(port) ;
	inet.sin_port = htons( in_port ) ;
	return nullptr ;
}

bool GNet::Address4::setZone( std::string_view /*ipv6_zone_name_or_scope_id*/ )
{
	return true ;
}

void GNet::Address4::setScopeId( unsigned long /*ipv6_scope_id*/ )
{
}

std::string GNet::Address4::displayString( bool /*ipv6_with_scope*/ ) const
{
	std::ostringstream ss ;
	ss << hostPartString() ;
	ss << Address4Imp::port_separator << port() ;
	return ss.str() ;
}

std::string GNet::Address4::hostPartString() const
{
	std::array<char,INET_ADDRSTRLEN+1U> buffer {} ;
	const void * vp = & m_inet.sin_addr ;
	const char * p = inet_ntop( af() , const_cast<void*>(vp) , buffer.data() , buffer.size() ) ;
	if( p == nullptr )
		throw Address::Error( "inet_ntop() failure" ) ;
	buffer[buffer.size()-1U] = '\0' ;
	return { buffer.data() } ; // sic
}

std::string GNet::Address4::queryString() const
{
	G::StringArray parts = G::Str::splitIntoFields( hostPartString() , '.' ) ;
	std::reverse( parts.begin() , parts.end() ) ;
	return G::Str::join( "." , parts ) ;
}

bool GNet::Address4::validData( const sockaddr * addr , socklen_t len )
{
	return addr != nullptr && addr->sa_family == af() && len == sizeof(sockaddr_type) ;
}

bool GNet::Address4::validString( std::string_view s , std::string * reason_p )
{
	sockaddr_type inet {} ;
	const char * reason = setAddress( inet , s ) ;
	if( reason && reason_p )
		*reason_p = std::string(reason) ;
	return reason == nullptr ;
}

bool GNet::Address4::validStrings( std::string_view host_part , std::string_view port_part ,
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

bool GNet::Address4::validPort( unsigned int port )
{
	sockaddr_type inet {} ;
	const char * reason = setPort( inet , port ) ;
	return reason == nullptr ;
}

bool GNet::Address4::same( const Address4 & other , bool /*ipv6_compare_with_scope*/ ) const
{
	return
		m_inet.sin_family == af() &&
		other.m_inet.sin_family == af() &&
		sameAddr( m_inet.sin_addr , other.m_inet.sin_addr ) &&
		m_inet.sin_port == other.m_inet.sin_port ;
}

bool GNet::Address4::sameHostPart( const Address4 & other ) const
{
	return
		m_inet.sin_family == af() &&
		other.m_inet.sin_family == af() &&
		sameAddr( m_inet.sin_addr , other.m_inet.sin_addr ) ;
}

bool GNet::Address4::sameAddr( const ::in_addr & a , const ::in_addr & b )
{
	return a.s_addr == b.s_addr ;
}

unsigned int GNet::Address4::port() const
{
	return ntohs( m_inet.sin_port ) ;
}

unsigned long GNet::Address4::scopeId( unsigned long default_ ) const
{
	return default_ ;
}

#ifndef G_LIB_SMALL
const sockaddr * GNet::Address4::address() const
{
	return reinterpret_cast<const sockaddr*>(&m_inet) ;
}
#endif

sockaddr * GNet::Address4::address()
{
	return reinterpret_cast<sockaddr*>(&m_inet) ;
}

socklen_t GNet::Address4::length() noexcept
{
	return sizeof(sockaddr_type) ;
}

G::StringArray GNet::Address4::wildcards() const
{
	std::string ip_str = hostPartString() ;

	G::StringArray result ;
	result.reserve( 38U ) ;
	result.push_back( ip_str ) ;

	std::string_view ip_sv( ip_str.data() , ip_str.size() ) ;
	G::StringFieldT<std::string_view> part( ip_sv , "." , 1U ) ;
	std::string_view part0 = part() ;
	std::string_view part1 = (++part)() ;
	std::string_view part2 = (++part)() ;
	std::string_view part3 = (++part)() ;

	G_ASSERT( part.valid() ) ;
	if( !part.valid() )
		return result ;

	G_ASSERT( !(++part).valid() ) ;

	if( part0.empty() || !G::Str::isUInt(part0) ||
		part1.empty() || !G::Str::isUInt(part1) ||
		part2.empty() || !G::Str::isUInt(part2) ||
		part3.empty() || !G::Str::isUInt(part3) )
	{
		return result ;
	}

	unsigned int n0 = G::Str::toUInt(part0) ;
	unsigned int n1 = G::Str::toUInt(part1) ;
	unsigned int n2 = G::Str::toUInt(part2) ;
	unsigned int n3 = G::Str::toUInt(part3) ;

	std::string part_0_1_2 = std::string(part0.data(),part0.size()).append(1U,'.')
		.append(part1.data(),part1.size()).append(1U,'.')
		.append(part2.data(),part2.size()).append(1U,'.') ;
	std::string part_0_1 = std::string(part0.data(),part0.size()).append(1U,'.')
		.append(part1.data(),part1.size()).append(1U,'.') ;
	std::string part_0 = std::string(part0.data(),part0.size()).append(1U,'.') ;

	add( result , part_0_1_2 , n3 & 0xffU , "/32" ) ;
	add( result , part_0_1_2 , n3 & 0xfeU , "/31" ) ;
	add( result , part_0_1_2 , n3 & 0xfcU , "/30" ) ;
	add( result , part_0_1_2 , n3 & 0xf8U , "/29" ) ;
	add( result , part_0_1_2 , n3 & 0xf0U , "/28" ) ;
	add( result , part_0_1_2 , n3 & 0xe0U , "/27" ) ;
	add( result , part_0_1_2 , n3 & 0xc0U , "/26" ) ;
	add( result , part_0_1_2 , n3 & 0x80U , "/25" ) ;
	add( result , part_0_1_2 , 0 , "/24" ) ;
	add( result , part_0_1_2 , "*" ) ;
	add( result , part_0_1 , n2 & 0xfeU , ".0/23" ) ;
	add( result , part_0_1 , n2 & 0xfcU , ".0/22" ) ;
	add( result , part_0_1 , n2 & 0xfcU , ".0/21" ) ;
	add( result , part_0_1 , n2 & 0xf8U , ".0/20" ) ;
	add( result , part_0_1 , n2 & 0xf0U , ".0/19" ) ;
	add( result , part_0_1 , n2 & 0xe0U , ".0/18" ) ;
	add( result , part_0_1 , n2 & 0xc0U , ".0/17" ) ;
	add( result , part_0_1 , 0 , ".0/16" ) ;
	add( result , part_0_1 , "*.*" ) ;
	add( result , part_0 , n1 & 0xfeU , ".0.0/15" ) ;
	add( result , part_0 , n1 & 0xfcU , ".0.0/14" ) ;
	add( result , part_0 , n1 & 0xf8U , ".0.0/13" ) ;
	add( result , part_0 , n1 & 0xf0U , ".0.0/12" ) ;
	add( result , part_0 , n1 & 0xe0U , ".0.0/11" ) ;
	add( result , part_0 , n1 & 0xc0U , ".0.0/10" ) ;
	add( result , part_0 , n1 & 0x80U , ".0.0/9" ) ;
	add( result , part_0 , 0 , ".0.0/8" ) ;
	add( result , part_0 , "*.*.*" ) ;
	add( result , n0 & 0xfeU , ".0.0.0/7" ) ;
	add( result , n0 & 0xfcU , ".0.0.0/6" ) ;
	add( result , n0 & 0xf8U , ".0.0.0/5" ) ;
	add( result , n0 & 0xf0U , ".0.0.0/4" ) ;
	add( result , n0 & 0xe0U , ".0.0.0/3" ) ;
	add( result , n0 & 0xc0U , ".0.0.0/2" ) ;
	add( result , n0 & 0x80U , ".0.0.0/1" ) ;
	add( result , 0 , ".0.0.0/0" ) ;
	add( result , "*.*.*.*" ) ;

	return result ;
}

void GNet::Address4::add( G::StringArray & result , std::string_view head , unsigned int n , const char * tail )
{
	result.push_back( G::sv_to_string(head).append(G::Str::fromUInt(n)).append(tail) ) ;
}

void GNet::Address4::add( G::StringArray & result , unsigned int n , const char * tail )
{
	result.push_back( G::Str::fromUInt(n).append(tail) ) ;
}

void GNet::Address4::add( G::StringArray & result , std::string_view head , const char * tail )
{
	result.push_back( G::sv_to_string(head).append(tail) ) ;
}

void GNet::Address4::add( G::StringArray & result , const char * tail )
{
	result.emplace_back( tail ) ;
}

bool GNet::Address4::format( std::string_view s )
{
	// an independent check for the IPv4 dotted-quad format

	if( s.empty() || s.find_first_not_of("0123456789.") != std::string::npos ||
		std::count(s.begin(),s.end(),'.') != 3U || s.at(0U) == '.' ||
		s.at(s.size()-1U) == '.' || s.find("..") != std::string::npos )
			return false ;

	unsigned int n = 0U ;
	unsigned int z = static_cast<unsigned char>('0') ;
	for( char c : s )
	{
		unsigned int uc = static_cast<unsigned char>(c) ;
		n = c == '.' ? 0U : ( ( n * 10U ) + (uc-z) ) ;
		if( n >= 256U )
			return false ;
	}
	return true ;
}

unsigned int GNet::Address4::bits() const
{
	const unsigned long a = ntohl( m_inet.sin_addr.s_addr ) ;
	unsigned int count = 0U ;
	for( unsigned long mask = 0x80000000U ; mask && ( a & mask ) ; mask >>= 1U )
		count++ ;
	return count ;
}

bool GNet::Address4::isLocal( std::string & reason ) const
{
	if( isLoopback() || isLinkLocal() || isUniqueLocal() )
	{
		return true ;
	}
	else
	{
		reason = hostPartString().append( " is not in "
			"127.0.0.0/8, 169.254.0.0/16, 10.0.0.0/8, 172.16.0.0/12, or 192.168.0.0/16" ) ;
		return false ;
	}
}

bool GNet::Address4::isLoopback() const
{
	// RFC-1918, RFC-6890
	return ( ntohl(m_inet.sin_addr.s_addr) >> 24U ) == 127U ; // 127.0.0.0/8
}

bool GNet::Address4::isLinkLocal() const
{
	// RFC-3927, RFC-6890
	return ( ntohl(m_inet.sin_addr.s_addr) >> 16U ) == 0xA9FEU ; // 169.254.0.0/16
}

bool GNet::Address4::isUniqueLocal() const
{
	// RFC-1918, RFC-6890
	return
		( ntohl(m_inet.sin_addr.s_addr) >> 24U ) == 0x0AU || // 10.0.0.0/8
		( ntohl(m_inet.sin_addr.s_addr) >> 20U ) == 0xAC1U || // 172.16.0.0/12
		( ntohl(m_inet.sin_addr.s_addr) >> 16U ) == 0xC0A8U ; // 192.168.0.0/16
}

bool GNet::Address4::isMulticast() const
{
	// RFC-5771
	return ( ntohl(m_inet.sin_addr.s_addr) >> 28U ) == 0x0EU ; // [224-239].x.x.x
}

bool GNet::Address4::isAny() const
{
	return m_inet.sin_addr.s_addr == htonl(INADDR_ANY) ;
}

