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
/// \file glocation.cpp
///

#include "gdef.h"
#include "gstr.h"
#include "glocation.h"
#include "gresolver.h"
#include "gassert.h"
#include "glog.h"

GNet::Location::Location( const std::string & spec , int family ) :
	m_host(head(sockless(spec))) ,
	m_service(tail(sockless(spec))) ,
	m_address_valid(false) ,
	m_address(Address::defaultAddress()) ,
	m_family(family) ,
	m_update_time(0U) ,
	m_using_socks(false)
{
	m_using_socks = socksified( spec , m_socks_far_host , m_socks_far_port ) ;
	if( m_host.empty() )
		throw InvalidFormat( spec ) ; // eg. ":25"
	G_DEBUG( "GNet::Location::ctor: unresolved location [" << displayString() << "]" << (m_using_socks?" (using socks)":"") ) ;
}

GNet::Location::Location( const std::string & spec , int family , int ) : // nosocks() overload
	m_host(head(spec)) ,
	m_service(tail(spec)) ,
	m_address_valid(false) ,
	m_address(Address::defaultAddress()) ,
	m_family(family) ,
	m_update_time(0U) ,
	m_using_socks(false)
{
	G_DEBUG( "GNet::Location::ctor: unresolved location [" << displayString() << "]" ) ;
	if( m_host.empty() )
		throw InvalidFormat( spec ) ;
}

GNet::Location::Location( const std::string & socks_server , const std::string & far_server , int family ) : // socks() overload
	m_host(head(socks_server)) ,
	m_service(tail(socks_server)) ,
	m_address_valid(false) ,
	m_address(Address::defaultAddress()) ,
	m_family(family) ,
	m_update_time(0U) ,
	m_using_socks(true) ,
	m_socks_far_host(head(far_server)) ,
	m_socks_far_port(tail(far_server))
{
	if( m_socks_far_host.empty() || m_socks_far_port.empty() )
		throw InvalidFormat() ;
	if( !G::Str::isUInt(m_socks_far_port) )
		throw InvalidFormat( "invalid port number: [" + m_socks_far_port + "]" ) ;
	if( m_host.empty() )
		throw InvalidFormat( socks_server ) ;
	G_DEBUG( "GNet::Location::ctor: unresolved location [" << displayString() << "]" << " (using socks)" ) ;
}

GNet::Location GNet::Location::nosocks( const std::string & spec , int family )
{
	return { spec , family , 1 } ;
}

#ifndef G_LIB_SMALL
GNet::Location GNet::Location::socks( const std::string & socks_server , const std::string & far_server )
{
	return { socks_server , far_server , AF_UNSPEC } ;
}
#endif

std::string GNet::Location::sockless( const std::string & s )
{
	// "far-host:far-port@sockserver-host:sockserver-port"
	return G::Str::tail( s , s.find('@') , s ) ;
}

bool GNet::Location::socksified( const std::string & s , std::string & far_host_out , std::string & far_port_out )
{
	std::string::size_type pos = s.find('@') ;
	if( pos != std::string::npos )
	{
		std::string ss = G::Str::head( s , pos ) ;
		far_host_out = G::Str::head( ss , ss.rfind(':') ) ;
		far_port_out = G::Str::tail( ss , ss.rfind(':') ) ;
		G::Str::toUInt( far_port_out ) ; // throw if not a number
	}
	return pos != std::string::npos ;
}

std::string GNet::Location::head( const std::string & s )
{
	std::size_t pos = s.rfind( ':' ) ;
	std::string h = ( pos == std::string::npos && !s.empty() && s[0] == '/' ) ? s : G::Str::head( s , pos ) ; // eg. "/tmp/socket"
	if( h.size() > 1U && h.at(0U) == '[' && h.at(h.size()-1U) == ']' ) // eg. "[::1]:25"
		h = h.substr( 1U , h.size()-2U ) ;
	return h ;
}

std::string GNet::Location::tail( const std::string & s )
{
	return G::Str::tail( s , s.rfind(':') ) ;
}

std::string GNet::Location::host() const
{
	return m_host ;
}

std::string GNet::Location::service() const
{
	return m_service ;
}

int GNet::Location::family() const
{
	return m_family ;
}

bool GNet::Location::resolveTrivially()
{
	std::string reason ;
	std::string address_string = G::Str::join( ":" , m_host , m_service ) ;
	if( !resolved() && Address::validString(address_string,&reason) )
	{
		update( Address::parse(address_string) ) ;
	}
	return resolved() ;
}

bool GNet::Location::resolved() const
{
	return m_address_valid ;
}

GNet::Address GNet::Location::address() const
{
	return m_address ;
}

void GNet::Location::update( const Address & address )
{
	if( !update(address,std::nothrow) )
		throw InvalidFamily() ;
}

bool GNet::Location::update( const Address & address , std::nothrow_t )
{
	bool valid_family =
		address.family() == Address::Family::ipv4 ||
		address.family() == Address::Family::ipv6 ||
		address.family() == Address::Family::local ;

	if( !valid_family || ( m_family != AF_UNSPEC && address.af() != m_family ) )
		return false ;

	m_address = address ;
	m_family = address.af() ; // not enum
	m_address_valid = true ;
	m_update_time = G::SystemTime::now() ;
	G_DEBUG( "GNet::Location::ctor: resolved location [" << displayString() << "]" ) ;
	return true ;
}

std::string GNet::Location::displayString() const
{
	if( resolved() )
	{
		return address().displayString() ;
	}
	else if( m_host.find('/') == 0U )
	{
		return m_host ;
	}
	else
	{
		const char * ipvx = m_family == AF_UNSPEC ? "ip" : ( m_family == AF_INET ? "ipv4" : "ipv6" ) ;
		return m_host + "/" + m_service + "/" + ipvx ;
	}
}

#ifndef G_LIB_SMALL
G::SystemTime GNet::Location::updateTime() const
{
	return m_update_time ;
}
#endif

bool GNet::Location::socks() const
{
	return m_using_socks ;
}

unsigned int GNet::Location::socksFarPort() const
{
	G_ASSERT( m_socks_far_port.empty() || G::Str::isUInt(m_socks_far_port) ) ;
	return m_socks_far_port.empty() ? 0U : G::Str::toUInt(m_socks_far_port) ;
}

std::string GNet::Location::socksFarHost() const
{
	return m_socks_far_host ;
}

