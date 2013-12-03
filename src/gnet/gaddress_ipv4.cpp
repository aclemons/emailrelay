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
// gaddress_ipv4.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gaddress.h"
#include "gstrings.h"
#include "gstr.h"
#include "gassert.h"
#include "gdebug.h"
#include <climits>
#include <algorithm> // std::swap
#include <utility> // std::swap
#include <sys/types.h>

/// \class GNet::AddressImp
/// A pimple-pattern implementation class for GNet::Address.
/// 
class GNet::AddressImp 
{
public:
	typedef sockaddr general_type ;
	typedef sockaddr_in address_type ;
	typedef sockaddr storage_type ;
	/// Used by GNet::AddressImp to cast between sockaddr and sockaddr_in.
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
	void setPort( unsigned int port ) ;

	static bool validString( const std::string & s , std::string * reason_p = NULL ) ;
	static bool validPort( unsigned int port ) ;

	bool same( const AddressImp & other ) const ;
	bool sameHost( const AddressImp & other ) const ;
	static G::Strings wildcards( const std::string & display_string ) ;

	std::string displayString() const ;
	std::string hostString() const ;

private:
	void init() ;
	static unsigned short family() ;
	void set( const sockaddr * general ) ;
	bool setAddress( const std::string & display_string , std::string & reason ) ;
	static bool validPart( const std::string & s ) ;
	static bool validPortNumber( const std::string & s ) ;
	static bool validNumber( const std::string & s ) ;
	void setHost( const hostent & h ) ;
	static bool sameAddr( const ::in_addr & a , const ::in_addr & b ) ;
	static char portSeparator() ;
	static void add( G::Strings & , const std::string & , unsigned int , const char * ) ;
	static void add( G::Strings & , const std::string & , const char * ) ;

private:
	Sockaddr m_inet ;
} ;

// ===

/// \class GNet::AddressStorageImp
/// A pimple-pattern implementation class for GNet::AddressStorage.
/// 
class GNet::AddressStorageImp 
{
public:
	AddressImp::Sockaddr u ;
	socklen_t n ;
} ;

// ===

unsigned short GNet::AddressImp::family()
{
	return AF_INET ;
}

void GNet::AddressImp::init()
{
	static address_type zero ;
	m_inet.specific = zero ;
	m_inet.specific.sin_family = family() ;
	m_inet.specific.sin_port = 0 ;
}

GNet::AddressImp::AddressImp( unsigned int port )
{
	init() ;
	m_inet.specific.sin_addr.s_addr = htonl(INADDR_ANY);
	setPort( port ) ;
}

GNet::AddressImp::AddressImp( unsigned int port , Address::Localhost )
{
	init() ;
	m_inet.specific.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	setPort( port ) ;
}

GNet::AddressImp::AddressImp( unsigned int port , Address::Broadcast )
{
	init() ;
	m_inet.specific.sin_addr.s_addr = htonl(INADDR_BROADCAST);
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
	m_inet.specific.sin_port = s.s_port ;
}

GNet::AddressImp::AddressImp( const servent & s )
{
	init() ;
	m_inet.specific.sin_addr.s_addr = htonl(INADDR_ANY);
	m_inet.specific.sin_port = s.s_port ;
}

GNet::AddressImp::AddressImp( const sockaddr * addr , size_t len )
{
	init() ;

	if( addr == NULL )
		throw Address::Error() ;

	if( addr->sa_family != family() || len != sizeof(address_type) )
		throw Address::BadFamily() ;

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

	const std::string::size_type pos = display_string.rfind(portSeparator()) ;
	std::string host_part = display_string.substr(0U,pos) ;
	std::string port_part = display_string.substr(pos+1U) ;

	m_inet.specific.sin_family = family() ;
	m_inet.specific.sin_addr.s_addr = ::inet_addr( host_part.c_str() ) ;
	setPort( G::Str::toUInt(port_part) ) ;

	G_ASSERT( displayString() == display_string ) ;
	return true ;
}

void GNet::AddressImp::setPort( unsigned int port )
{
	if( ! validPort(port) )
		throw Address::Error( "invalid port number" ) ;

	const g_port_t in_port = static_cast<g_port_t>(port) ;
	m_inet.specific.sin_port = htons( in_port ) ;
}

void GNet::AddressImp::setHost( const hostent & h )
{
	if( h.h_addrtype != family() || h.h_addr_list[0] == NULL )
		throw Address::BadFamily() ;

	const char * first = h.h_addr_list[0U] ;
	const in_addr * raw = reinterpret_cast<const in_addr*>(first) ;
	m_inet.specific.sin_addr = *raw ;
}

std::string GNet::AddressImp::displayString() const
{
	std::ostringstream ss ;
	ss << hostString() ;
	ss << portSeparator() << port() ;
	return ss.str() ;
}

std::string GNet::AddressImp::hostString() const
{
	std::ostringstream ss ;
	ss << ::inet_ntoa(m_inet.specific.sin_addr) ;
	return ss.str() ;
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

	const std::string::size_type pos = s.rfind(portSeparator()) ;
	if( pos == std::string::npos )
	{
		reason = "no port separator" ;
		return false ;
	}

	std::string port_part = s.substr(pos+1U) ;
	if( !validPortNumber(port_part) )
	{
		reason = std::string() + "invalid port number: [" + port_part + "]" ;
		return false ;
	}

	std::string host_part = s.substr(0U,pos) ;

	// (could test the string by passing it
	// to inet_addr() here, but the man page points
	// out that the error return value is not
	// definitive)

	G::Strings parts ;
	G::Str::splitIntoFields( host_part , parts , "." ) ;
	if( parts.size() != 4U )
	{
		reason = "invalid number of dotted parts" ;
		return false ;
	}

	G::Strings::iterator p = parts.begin() ;
	reason = "invalid dotted part" ;
	if( ! validPart(*p) ) return false ; p++ ;
	if( ! validPart(*p) ) return false ; p++ ;
	if( ! validPart(*p) ) return false ; p++ ;
	if( ! validPart(*p) ) return false ;

	reason = "" ;
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

bool GNet::AddressImp::validPart( const std::string & s )
{
	return validNumber(s) && G::Str::toUInt(s,true) <= 255U ;
}

bool GNet::AddressImp::same( const AddressImp & other ) const
{
	return
		m_inet.specific.sin_family == other.m_inet.specific.sin_family &&
		m_inet.specific.sin_family == family() &&
		sameAddr( m_inet.specific.sin_addr , other.m_inet.specific.sin_addr ) &&
		m_inet.specific.sin_port == other.m_inet.specific.sin_port ;
}

bool GNet::AddressImp::sameHost( const AddressImp & other ) const
{
	return
		m_inet.specific.sin_family == other.m_inet.specific.sin_family &&
		m_inet.specific.sin_family == family() &&
		sameAddr( m_inet.specific.sin_addr , other.m_inet.specific.sin_addr ) ;
}

bool GNet::AddressImp::sameAddr( const ::in_addr & a , const ::in_addr & b )
{
	return a.s_addr == b.s_addr ;
}

unsigned int GNet::AddressImp::port() const
{
	return ntohs( m_inet.specific.sin_port ) ;
}

const sockaddr * GNet::AddressImp::raw() const
{
	return & m_inet.general ;
}

sockaddr * GNet::AddressImp::raw()
{
	return & m_inet.general ;
}

void GNet::AddressImp::set( const sockaddr * general )
{
	m_inet.general = *general ;
}

char GNet::AddressImp::portSeparator()
{
	return ':' ;
}

G::Strings GNet::AddressImp::wildcards( const std::string & display_string )
{
	G::Strings result ;
	result.push_back( display_string ) ;

	G::StringArray part ;
	G::Str::splitIntoFields( display_string , part , "." ) ;

	G_ASSERT( part.size() == 4U ) ;
	if( part.size() != 4U || 
		!G::Str::isUInt(part[0]) ||
		!G::Str::isUInt(part[1]) ||
		!G::Str::isUInt(part[2]) ||
		!G::Str::isUInt(part[3]) )
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

void GNet::AddressImp::add( G::Strings & result , const std::string & head , unsigned int n , const char * tail )
{
	std::string s = head ;
	s.append( G::Str::fromUInt( n ) ) ;
	s.append( tail ) ;
	result.push_back( s ) ;
}

void GNet::AddressImp::add( G::Strings & result , const std::string & head , const char * tail )
{
	result.push_back( head + tail ) ;
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

std::string GNet::Address::displayString( bool with_port , bool ) const
{
	return with_port ? m_imp->displayString() : m_imp->hostString() ;
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

unsigned long GNet::Address::scopeId( unsigned long default_ ) const
{
	return default_ ;
}

bool GNet::Address::validPort( unsigned int port )
{
	return AddressImp::validPort( port ) ;
}

int GNet::Address::defaultDomain()
{
	return PF_INET ;
}

int GNet::Address::domain() const
{
	return defaultDomain() ;
}

G::Strings GNet::Address::wildcards() const
{
	return AddressImp::wildcards( displayString(false) ) ;
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

