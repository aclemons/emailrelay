//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gnet.h"
#include "gaddress.h"
#include "gstrings.h"
#include "gstr.h"
#include "gexception.h"
#include "gassert.h"
#include "gdebug.h"
#include <algorithm> // std::swap
#include <utility> // std::swap
#include <climits>
#include <sys/types.h>
#include <vector>
#include <iomanip>
#include <sstream>

/// \class GNet::AddressImp
/// A pimple-pattern implementation class for GNet::Address.
/// 
class GNet::AddressImp 
{
public:
	typedef sockaddr general_type ;
	typedef sockaddr_in6 address_type ;
	typedef sockaddr_storage storage_type ;
	/// Used by GNet::AddressImp to cast between sockaddr and sockaddr_in6.
	union Sockaddr 
		{ address_type specific ; general_type general ; storage_type storage ; } ;

	explicit AddressImp( unsigned int port ) ; // (not in_port_t -- see validPort(), setPort() etc)
	explicit AddressImp( const servent & s ) ;
	explicit AddressImp( const std::string & s ) ;
	AddressImp( const std::string & s , unsigned int port ) ;
	AddressImp( unsigned int port , Address::Localhost ) ;
	AddressImp( unsigned int port , Address::Broadcast ) ;
	AddressImp( const hostent & h , unsigned int port ) ;
	AddressImp( const hostent & h , const servent & s ) ;
	AddressImp( const sockaddr * addr , size_t len ) ;
	AddressImp( const AddressImp & other ) ;

	const sockaddr * raw() const ;
	sockaddr * raw() ;

	unsigned int port() const ;
	unsigned long scopeId() const ;
	void setPort( unsigned int port ) ;

	static bool validString( const std::string & s , std::string * reason_p = NULL ) ;
	static bool validPort( unsigned int port ) ;

	bool same( const AddressImp & other ) const ;
	bool sameHost( const AddressImp & other ) const ;

	std::string displayString( bool , bool ) const ;
	std::string hostString() const ;
	std::string networkString( unsigned int ) const ;
	static std::vector<unsigned char> networkMask( unsigned int ) ;

private:
	void init() ;
	static unsigned short family() ;
	void set4( const sockaddr * general ) ;
	bool setAddress( const std::string & display_string , std::string & reason ) ;
	static bool validPortNumber( const std::string & s ) ;
	static bool validNumber( const std::string & s ) ;
	void setHost( const hostent & h ) ;
	static bool sameAddr( const ::in6_addr & a , const ::in6_addr & b ) ;
	static char portSeparator() ;

private:
	Sockaddr m_inet ;
} ;

// ===

class GNet::AddressStorageImp 
{
public:
	AddressImp::Sockaddr u ;
	socklen_t n ;
} ;

// ===

unsigned short GNet::AddressImp::family()
{
	return AF_INET6 ;
}

void GNet::AddressImp::init()
{
	static address_type zero ;
	m_inet.specific = zero ;
	m_inet.specific.sin6_family = family() ;
	m_inet.specific.sin6_flowinfo = 0 ;
	m_inet.specific.sin6_port = 0 ;

 #if defined(HAVE_SIN6_LEN) && HAVE_SIN6_LEN
	m_inet.specific.sin6_len = sizeof(m_inet.specific) ;
 #endif
}

GNet::AddressImp::AddressImp( unsigned int port )
{
	init() ;
	m_inet.specific.sin6_addr = in6addr_any ;
	setPort( port ) ;
}

GNet::AddressImp::AddressImp( unsigned int port , Address::Localhost )
{
	init() ;
	m_inet.specific.sin6_addr = in6addr_loopback ;
	setPort( port ) ;
}

GNet::AddressImp::AddressImp( unsigned int , Address::Broadcast )
{
	throw G::Exception( "broadcast addresses not implemented for ipv6" ) ; // for now
}

GNet::AddressImp::AddressImp( const hostent & h , unsigned int port )
{
	init() ;
	setHost( h ) ;
	setPort( port ) ;
}

GNet::AddressImp::AddressImp( const hostent & h , const servent & s )
{
	init() ;
	setHost( h ) ;
	m_inet.specific.sin6_port = s.s_port ;
}

GNet::AddressImp::AddressImp( const servent & s )
{
	init() ;
	m_inet.specific.sin6_addr = in6addr_any ;
	m_inet.specific.sin6_port = s.s_port ;
}

GNet::AddressImp::AddressImp( const sockaddr * addr , size_t len )
{
	init() ;

	if( addr == NULL )
		throw Address::Error() ;

	Sockaddr u ;
	bool ipv6 = addr->sa_family == family() && len == sizeof(u.specific) ;
	if( !ipv6 )
	{
		std::ostringstream ss ;
		ss << addr->sa_family ;
		throw Address::BadFamily( ss.str() ) ;
	}

	u.general = * addr ;
	m_inet.specific = u.specific ;
}

GNet::AddressImp::AddressImp( const AddressImp & other )
{
	m_inet.specific = other.m_inet.specific ;
}

GNet::AddressImp::AddressImp( const std::string & s , unsigned int port )
{
	init() ;

	std::string reason ;
	if( ! setAddress( s + portSeparator() + "0" , reason ) )
		throw Address::BadString( reason + ": " + s ) ;

	setPort( port ) ;
}

GNet::AddressImp::AddressImp( const std::string & s )
{
	init() ;

	std::string reason ;
	if( ! setAddress( s , reason ) )
		throw Address::BadString( reason + ": " + s ) ;
}

bool GNet::AddressImp::setAddress( const std::string & display_string , std::string & reason )
{
	if( !validString(display_string,&reason) )
		return false ;

	const size_t pos = display_string.rfind(portSeparator()) ;
	std::string port_part = display_string.substr(pos+1U) ;
	std::string host_part = display_string.substr(0U,pos) ;

	m_inet.specific.sin6_family = family() ;

	void * vp = & m_inet.specific.sin6_addr ;
	int rc = ::inet_pton( family() , host_part.c_str() , vp ) ;
	if( rc != 1 )
		return false ; // never gets here

	setPort( G::Str::toUInt(port_part) ) ;
	return true ;
}

void GNet::AddressImp::setPort( unsigned int port )
{
	if( ! validPort(port) )
		throw Address::Error( "invalid port number" ) ;

	const g_port_t in_port = static_cast<g_port_t>(port) ;
	m_inet.specific.sin6_port = htons( in_port ) ;
}

void GNet::AddressImp::setHost( const hostent & h )
{
	if( h.h_addrtype != family() || h.h_addr_list[0U] == NULL )
		throw Address::BadFamily( "setHost" ) ;

	const char * first = h.h_addr_list[0U] ;
	const in6_addr * raw = reinterpret_cast<const in6_addr*>(first) ;
	m_inet.specific.sin6_addr = *raw ;
}

std::string GNet::AddressImp::displayString( bool with_port , bool with_scope_id ) const
{
	std::ostringstream ss ;
	ss << hostString() ;
	if( with_scope_id )
		ss << "%" << scopeId() ;
	if( with_port )
		ss << portSeparator() << port() ;
	return ss.str() ;
}

std::string GNet::AddressImp::hostString() const
{
	char buffer[INET6_ADDRSTRLEN+1U] ;
	const void * vp = & m_inet.specific.sin6_addr ;
	const char * p = ::inet_ntop( family() , vp , buffer , sizeof(buffer) ) ;
	if( p == NULL )
		throw Address::Error( "inet_ntop() failure" ) ;
	return std::string(buffer) ;
}

std::vector<unsigned char> GNet::AddressImp::networkMask( unsigned int bits )
{
	bits = bits > 128U ? 128U : bits ;
	std::vector<unsigned char> result ;
	const unsigned char ff = 0xff ;
	for( unsigned int i = 0U ; i < 16U ; i++ )
		result.push_back( (i*8U) > bits ? ff : ( ((i+1U)*8U) < bits ? 0U : (ff << (bits-(i*8U))) ) ) ;
	return result ;
}

std::string GNet::AddressImp::networkString( unsigned int bits ) const
{
	std::vector<unsigned char> mask_array = networkMask( bits ) ;
	std::ostringstream ss ;
	const char * sep = "" ;
	for( unsigned int i = 0U ; i < 16U ; i++ )
	{
		unsigned int value = m_inet.specific.sin6_addr.s6_addr[i] ;
		unsigned int mask = mask_array[15U-i] ;
		ss << sep << std::setw(2U) << std::setfill('0') << std::hex << (value & mask) ;
		sep = (i&1) ? ":" : "" ;
	}
	return G::Str::lower(ss.str()) ;
}

bool GNet::AddressImp::validPort( unsigned int port )
{
	return port <= 0xFFFFU ; // port numbers are now explicitly 16 bits, not short ints
}

bool GNet::AddressImp::validString( const std::string & s , std::string * reason_p )
{
	std::string buffer ;
	if( reason_p == NULL ) reason_p = &buffer ;
	std::string & reason = *reason_p ;

	const size_t pos = s.rfind(portSeparator()) ;
	if( pos == std::string::npos )
	{
		reason = "no port separator" ;
		return false ;
	}

	std::string port_part = s.substr(pos+1U) ;
	if( !validPortNumber(port_part) )
	{
		reason = "invalid port number" ;
		return false ;
	}

	std::string host_part = s.substr(0U,pos) ;
	address_type inet ;
	void * vp = & inet.sin6_addr ;
	int rc = ::inet_pton( family() , host_part.c_str() , vp ) ;
	const bool ok = rc == 1 ;
	if( !ok )
	{
		reason = "invalid format" ;
		return false ;
	}

	return true ;
}

bool GNet::AddressImp::validPortNumber( const std::string & s )
{
	return validNumber(s) && G::Str::isUInt(s) && validPort(G::Str::toUInt(s)) ;
}

bool GNet::AddressImp::validNumber( const std::string & s )
{
	return s.length() != 0U && G::Str::isNumeric(s) ;
}

bool GNet::AddressImp::same( const AddressImp & other ) const
{
	return
		m_inet.specific.sin6_family == other.m_inet.specific.sin6_family &&
		m_inet.specific.sin6_family == family() &&
		sameAddr( m_inet.specific.sin6_addr , other.m_inet.specific.sin6_addr ) &&
		m_inet.specific.sin6_port == other.m_inet.specific.sin6_port ;
}

bool GNet::AddressImp::sameHost( const AddressImp & other ) const
{
	return
		m_inet.specific.sin6_family == other.m_inet.specific.sin6_family &&
		m_inet.specific.sin6_family == family() &&
		sameAddr( m_inet.specific.sin6_addr , other.m_inet.specific.sin6_addr ) ;
}

bool GNet::AddressImp::sameAddr( const ::in6_addr & a , const ::in6_addr & b )
{
	for( size_t i = 0 ; i < 16U ; i++ )
	{
		if( a.s6_addr[i] != b.s6_addr[i] )
			return false ;
	}
	return true ;
}

unsigned int GNet::AddressImp::port() const
{
	return ntohs( m_inet.specific.sin6_port ) ;
}

unsigned long GNet::AddressImp::scopeId() const
{
	return m_inet.specific.sin6_scope_id ;
}

const sockaddr * GNet::AddressImp::raw() const
{
	return &m_inet.general ;
}

sockaddr * GNet::AddressImp::raw()
{
	return &m_inet.general ;
}

char GNet::AddressImp::portSeparator()
{
	return ':' ;
}

// ===

GNet::Address GNet::Address::invalidAddress()
{
	return Address( 0U ) ;
}

GNet::Address GNet::Address::localhost( unsigned int port )
{
	return Address( port , Localhost() ) ;
}

GNet::Address GNet::Address::broadcastAddress( unsigned int port )
{
	return Address( port , Broadcast() ) ;
}

GNet::Address::Address( unsigned int port ) :
	m_imp( new AddressImp(port) )
{
}

GNet::Address::Address( unsigned int port , Localhost dummy ) :
	m_imp( new AddressImp(port,dummy) )
{
}

GNet::Address::Address( unsigned int port , Broadcast dummy ) :
	m_imp( new AddressImp(port,dummy) )
{
}

GNet::Address::Address( const hostent & h , unsigned int port ) :
	m_imp( new AddressImp(h,port) )
{
}

GNet::Address::Address( const hostent & h , const servent & s ) :
	m_imp( new AddressImp(h,s) )
{
}

GNet::Address::Address( const servent & s ) :
	m_imp( new AddressImp(s) )
{
}

GNet::Address::Address( const AddressStorage & storage ) :
	m_imp( new AddressImp(storage.p(),storage.n()) )
{
}

GNet::Address::Address( const sockaddr * addr , socklen_t len ) :
	m_imp( new AddressImp(addr,len) )
{
}

GNet::Address::Address( const std::string & s ) :
	m_imp( new AddressImp(s) )
{
}

GNet::Address::Address( const std::string & s , unsigned int port ) :
	m_imp( new AddressImp(s,port) )
{
}

GNet::Address::Address( const Address & other ) :
	m_imp( new AddressImp(*other.m_imp) )
{
}

void GNet::Address::operator=( const Address & addr )
{
	Address temp( addr ) ;
	std::swap( m_imp , temp.m_imp ) ;
}

GNet::Address::~Address()
{
	delete m_imp ;
}

void GNet::Address::setPort( unsigned int port )
{
	m_imp->setPort( port ) ;
}

bool GNet::Address::operator==( const Address & other ) const
{
	return m_imp->same(*other.m_imp) ;
}

bool GNet::Address::isLocal( std::string & reason ) const
{
	if( sameHost(localhost()) )
		return true ;

	std::ostringstream ss ;
	ss << displayString(false) << " is not " << localhost().displayString(false) ;
	reason = ss.str() ;

	return false ;
}

bool GNet::Address::isLocal( std::string & reason , const Address & local_hint ) const
{
	if( sameHost(localhost()) || sameHost(local_hint) )
		return true ;

	std::ostringstream ss ;
	ss 
		<< displayString(false) << " is not one of " 
		<< localhost().displayString(false) << "," 
		<< local_hint.displayString(false) ; 
	reason = ss.str() ;

	return false ;
}

bool GNet::Address::sameHost( const Address & other ) const
{
	return m_imp->sameHost(*other.m_imp) ;
}

std::string GNet::Address::displayString( bool with_port , bool with_scope_id ) const
{
	return m_imp->displayString(with_port,with_scope_id) ;
}

std::string GNet::Address::hostString() const
{
	return m_imp->hostString() ;
}

bool GNet::Address::validString( const std::string & s , std::string * reason_p )
{
	return AddressImp::validString( s , reason_p ) ;
}

sockaddr * GNet::Address::address()
{
	return m_imp->raw() ;
}

const sockaddr * GNet::Address::address() const
{
	return m_imp->raw() ;
}

socklen_t GNet::Address::length() const
{
	return sizeof(AddressImp::address_type) ;
}

unsigned int GNet::Address::port() const
{
	return m_imp->port() ;
}

unsigned long GNet::Address::scopeId( unsigned long ) const
{
	return m_imp->scopeId() ;
}

bool GNet::Address::validPort( unsigned int port )
{
	return AddressImp::validPort( port ) ;
}

int GNet::Address::defaultDomain()
{
	return PF_INET6 ;
}

int GNet::Address::domain() const
{
	return defaultDomain() ;
}

G::Strings GNet::Address::wildcards() const
{
	G::Strings result ;
	for( unsigned int i = 0U ; i < 128U ; i++ )
	{
		std::ostringstream ss ;
		ss << m_imp->networkString(i) << "/" << i ;
		result.push_back( ss.str() ) ;
	}
	return result ;
}

// ===

GNet::AddressStorage::AddressStorage() :
	m_imp( new AddressStorageImp )
{
	m_imp->n = sizeof(AddressImp::Sockaddr) ;
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

