//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// gaddress_ipv6.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gaddress.h"
#include "gstrings.h"
#include "gstr.h"
#include "gassert.h"
#include "gdebug.h"
#include <climits>
#include <sys/types.h>

// Class: GNet::AddressImp
// Description: A pimple-pattern implementation class for GNet::Address.
//
class GNet::AddressImp 
{
public:
	typedef sockaddr_in6 address_type ;
	union Sockaddr // Used by GNet::AddressImp to cast between sockaddr and sockaddr_in6.
		{ address_type specific ; struct sockaddr general ; } ;

	explicit AddressImp( unsigned int port ) ; // (not in_port_t -- see validPort(), setPort() etc)
	explicit AddressImp( const servent & s ) ;
	explicit AddressImp( const std::string & s ) ;
	AddressImp( const std::string & s , unsigned int port ) ;
	AddressImp( unsigned int port , Address::Localhost ) ;
	AddressImp( const hostent & h , unsigned int port ) ;
	AddressImp( const hostent & h , const servent & s ) ;
	AddressImp( const sockaddr * addr , size_t len ) ;
	AddressImp( const AddressImp & other ) ;

	const sockaddr * raw() const ;
	sockaddr * raw() ;

	unsigned int port() const ;
	void setPort( unsigned int port ) ;

	static bool validString( const std::string & s , std::string * reason_p = NULL ) ;
	static bool validPort( unsigned int port ) ;

	bool same( const AddressImp & other ) const ;
	bool sameHost( const AddressImp & other ) const ;

	std::string displayString() const ;
	std::string hostString() const ;

private:
	void init() ;
	void set( const sockaddr * general ) ;
	bool setAddress( const std::string & display_string , std::string & reason ) ;

	static bool validPortNumber( const std::string & s ) ;
	static bool validNumber( const std::string & s ) ;
	void setHost( const hostent & h ) ;
	static bool sameAddr( const ::in6_addr & a , const ::in6_addr & b ) ;

private:
	address_type m_inet ;
	static char m_port_separator ;
} ;

char GNet::AddressImp::m_port_separator = ':' ;

void GNet::AddressImp::init()
{
	::memset( &m_inet, 0, sizeof(m_inet) );
	m_inet.sin6_family = AF_INET6 ;
	m_inet.sin6_flowinfo = 0 ;
	m_inet.sin6_port = 0 ;

 #if defined( SIN6_LEN )
	m_inet.sin6_len = sizeof(m_inet) ;
 #endif
}

GNet::AddressImp::AddressImp( unsigned int port )
{
	init() ;
	m_inet.sin6_addr = in6addr_any ;
	setPort( port ) ;
}

GNet::AddressImp::AddressImp( unsigned int port , Address::Localhost )
{
	init() ;
	m_inet.sin6_addr = in6addr_loopback ;
	setPort( port ) ;
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
	m_inet.sin6_port = s.s_port ;
}

GNet::AddressImp::AddressImp( const servent & s )
{
	init() ;
	m_inet.sin6_addr = in6addr_any ;
	m_inet.sin6_port = s.s_port ;
}

GNet::AddressImp::AddressImp( const sockaddr * addr , size_t len )
{
	init() ;

	if( addr == NULL )
		throw Address::Error() ;

	if( addr->sa_family != AF_INET6 || len != sizeof(m_inet.sin6_addr) )
		throw Address::BadFamily( std::stringstream() << addr->sa_family ) ;

	set( addr ) ;
}

GNet::AddressImp::AddressImp( const AddressImp & other )
{
	m_inet = other.m_inet ;
}

GNet::AddressImp::AddressImp( const std::string & s , unsigned int port )
{
	init() ;

	std::string reason ;
	if( ! setAddress( s + m_port_separator + "0" , reason ) )
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

	const size_t pos = display_string.rfind(m_port_separator) ;
	std::string port_part = display_string.substr(pos+1U) ;
	std::string host_part = display_string.substr(0U,pos) ;

	m_inet.sin6_family = AF_INET6 ;

	void * vp = & m_inet.sin6_addr ;
	int rc = ::inet_pton( AF_INET6 , host_part.c_str() , vp ) ;
	if( rc != 1 )
		return false ; // never gets here

	setPort( G::Str::toUInt(port_part) ) ;

	// sanity check
	{
		std::string s1 = displayString() ; G::Str::toLower(s1) ;
		std::string s2 = display_string ; G::Str::toLower(s2) ;
		if( s1 != s2 )
		{
			G_ERROR( "GNet::AddressImp::setAddress: \"" << s1 << "\" != \"" << s2 << "\"" ) ;
			throw Address::Error( "bad address string conversion" ) ;
		}
	}

	return true ;
}

void GNet::AddressImp::setPort( unsigned int port )
{
	if( ! validPort(port) )
		throw Address::Error( "invalid port number" ) ;

	const g_port_t in_port = static_cast<g_port_t>(port) ;
	m_inet.sin6_port = htons( in_port ) ;
}

void GNet::AddressImp::setHost( const hostent & h )
{
	if( h.h_addrtype != AF_INET6 || h.h_addr_list[0U] == NULL )
		throw Address::BadFamily( "setHost" ) ;

	const char * first = h.h_addr_list[0U] ;
	const in6_addr * raw = reinterpret_cast<const in6_addr*>(first) ;
	m_inet.sin6_addr = *raw ;
}

std::string GNet::AddressImp::displayString() const
{
	std::stringstream ss ;
	ss << hostString() ;
	ss << m_port_separator << port() ;
	return ss.str() ;
}

std::string GNet::AddressImp::hostString() const
{
	char buffer[INET6_ADDRSTRLEN+1U] ;
	const void * vp = & m_inet.sin6_addr ;
	const char * p = ::inet_ntop( AF_INET6 , vp , buffer , sizeof(buffer) ) ;
	if( p == NULL )
		throw Address::Error( "inet_ntop() failure" ) ;
	return std::string(buffer) ;
}

//static
bool GNet::AddressImp::validPort( unsigned int port )
{
	return port <= 0xFFFFU ; // port numbers are now explicitly 16 bits, not short ints
}

//static
bool GNet::AddressImp::validString( const std::string & s , std::string * reason_p )
{
	std::string buffer ;
	if( reason_p == NULL ) reason_p = &buffer ;
	std::string & reason = *reason_p ;

	const size_t pos = s.rfind(m_port_separator) ;
	if( pos == std::string::npos )
	{
		reason = "no port separator" ;
		return false ;
	}

	std::string port_part = s.substr(pos+1U) ;
	if( !validPortNumber(port_part) )
	{
		reason = "invalid port" ;
		return false ;
	}

	std::string host_part = s.substr(0U,pos) ;
	address_type inet ;
	void * vp = & inet.sin6_addr ;
	int rc = ::inet_pton( AF_INET6 , host_part.c_str() , vp ) ;
	const bool ok = rc == 1 ;
	if( !ok )
	{
		reason = "invalid format" ;
		return false ;
	}

	return true ;
}

//static
bool GNet::AddressImp::validPortNumber( const std::string & s )
{
	return validNumber(s) && G::Str::isUInt(s) && validPort(G::Str::toUInt(s)) ;
}

//static
bool GNet::AddressImp::validNumber( const std::string & s )
{
	return s.length() != 0U && G::Str::isNumeric(s) ;
}

bool GNet::AddressImp::same( const AddressImp & other ) const
{
	return
		m_inet.sin6_family == other.m_inet.sin6_family &&
		m_inet.sin6_family == AF_INET6 &&
		sameAddr( m_inet.sin6_addr , other.m_inet.sin6_addr ) &&
		m_inet.sin6_port == other.m_inet.sin6_port ;
}

bool GNet::AddressImp::sameHost( const AddressImp & other ) const
{
	return
		m_inet.sin6_family == other.m_inet.sin6_family &&
		m_inet.sin6_family == AF_INET6 &&
		sameAddr( m_inet.sin6_addr , other.m_inet.sin6_addr ) ;
}

//static
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
	return ntohs( m_inet.sin6_port ) ;
}

const sockaddr * GNet::AddressImp::raw() const
{
	return reinterpret_cast<const sockaddr*>(&m_inet) ;
}

sockaddr * GNet::AddressImp::raw()
{
	return reinterpret_cast<sockaddr*>(&m_inet) ;
}

void GNet::AddressImp::set( const sockaddr * general )
{
	Sockaddr u ;
	u.general = * general ;
	m_inet = u.specific ;
}

// ===

//static
GNet::Address GNet::Address::invalidAddress()
{
	return Address( 0U ) ;
}

GNet::Address::Address( unsigned int port ) :
	m_imp( new AddressImp(port) )
{
}

GNet::Address::Address( unsigned int port , Localhost dummy ) :
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

GNet::Address::Address( const sockaddr *addr , int len ) :
	m_imp( new AddressImp(addr,len) )
{
}

GNet::Address::Address( const Address & other ) :
	m_imp( new AddressImp(*other.m_imp) )
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

GNet::Address::~Address()
{
	delete m_imp ;
}

void GNet::Address::setPort( unsigned int port )
{
	m_imp->setPort( port ) ;
}

void GNet::Address::operator=( const Address & addr )
{
	delete m_imp ;
	m_imp = NULL ;
	m_imp = new AddressImp(*addr.m_imp) ;
}

bool GNet::Address::operator==( const Address & other ) const
{
	return m_imp->same(*other.m_imp) ;
}

bool GNet::Address::sameHost( const Address & other ) const
{
	return m_imp->sameHost(*other.m_imp) ;
}

std::string GNet::Address::displayString( bool with_port ) const
{
	return with_port ? m_imp->displayString() : m_imp->hostString() ;
}

std::string GNet::Address::hostString() const
{
	return m_imp->hostString() ;
}

//static
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

int GNet::Address::length() const
{
	return sizeof(AddressImp::address_type) ;
}

unsigned int GNet::Address::port() const
{
	return m_imp->port() ;
}

//static
bool GNet::Address::validPort( unsigned int port )
{
	return AddressImp::validPort( port ) ;
}

//static
GNet::Address GNet::Address::localhost( unsigned int port )
{
	return Address( port , Localhost() ) ;
}

