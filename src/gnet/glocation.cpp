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
// glocation.cpp
//

#include "gdef.h"
#include "gstr.h"
#include "glocation.h"
#include "gresolver.h"
#include "gassert.h"

namespace
{
	const char * host_service_separators = ":/" ;
}

GNet::Location::Location( const std::string & host , const std::string & service , int family ) :
	m_host(host) ,
	m_service(service) ,
	m_address_valid(false) ,
	m_address(Address::defaultAddress()) ,
	m_family(family) ,
	m_dgram(false) ,
	m_update_time(0U) ,
	m_socks(false) ,
	m_socks_far_port(0U)
{
	G_DEBUG( "GNet::Location::ctor: unresolved location [" << displayString() << "]" ) ;
}

GNet::Location::Location( const std::string & socks_host , const std::string & socks_service ,
	const std::string & far_host , const std::string & far_service , int family ) :
		m_host(socks_host) ,
		m_service(socks_service) ,
		m_address_valid(false) ,
		m_address(Address::defaultAddress()) ,
		m_family(family) ,
		m_dgram(false) ,
		m_update_time(0U) ,
		m_socks(!far_host.empty()) ,
		m_socks_far_host(far_host) ,
		m_socks_far_port(0U)
{
	if( far_host.empty() != far_service.empty() )
		throw InvalidFormat() ;
	if( m_socks && !G::Str::isUInt(far_service) )
		throw InvalidFormat( "invalid port number: [" + far_service + "]" ) ;
	if( m_socks )
		m_socks_far_port = G::Str::toUInt( far_service ) ;
	G_DEBUG( "GNet::Location::ctor: unresolved location [" << displayString() << "]" << (m_socks?" (using socks)":"") ) ;
}

GNet::Location::Location( const std::string & location_string , int family ) :
	m_host(head(sockless(location_string))) ,
	m_service(tail(sockless(location_string))) ,
	m_address_valid(false) ,
	m_address(Address::defaultAddress()) ,
	m_family(family) ,
	m_dgram(false) ,
	m_update_time(0U) ,
	m_socks(false) ,
	m_socks_far_port(0U)
{
	m_socks = socksified( location_string , m_socks_far_host , m_socks_far_port ) ;
	if( m_host.empty() || m_service.empty() )
		throw InvalidFormat( location_string ) ;
	G_DEBUG( "GNet::Location::ctor: unresolved location [" << displayString() << "]" << (m_socks?" (using socks)":"") ) ;
}

std::string GNet::Location::sockless( const std::string & s )
{
	// "far-host:far-port@sockserver-host:sockserver-port"
	return G::Str::tail( s , s.find('@') , s ) ;
}

bool GNet::Location::socksified( const std::string & s , std::string & far_host , unsigned int & far_port )
{
	std::string::size_type pos = s.find('@') ;
	if( pos != std::string::npos )
	{
		std::string ss = G::Str::head( s , pos ) ;
		far_host = G::Str::head( ss , ss.find_last_of(host_service_separators) ) ;
		far_port = G::Str::toUInt( G::Str::tail( ss , ss.find_last_of(host_service_separators) ) ) ;
	}
	return pos != std::string::npos ;
}

std::string GNet::Location::head( const std::string & s )
{
	std::string result = G::Str::head( s , s.find_last_of(host_service_separators) ) ;
	if( result.size() > 1U && result.at(0U) == '[' && result.at(result.size()-1U) == ']' )
		result = result.substr( 1U , result.size()-2U ) ;
	return result ;
}

std::string GNet::Location::tail( const std::string & s )
{
	return G::Str::tail( s , s.find_last_of(host_service_separators) ) ;
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

bool GNet::Location::dgram() const
{
	return m_dgram ;
}

void GNet::Location::resolveTrivially()
{
	std::string reason ;
	if( !resolved() && Address::validStrings(m_host,m_service,&reason) )
		update( Address(m_host,G::Str::toUInt(m_service)) , std::string() ) ;
}

bool GNet::Location::resolved() const
{
	return m_address_valid ;
}

GNet::Address GNet::Location::address() const
{
	return m_address ;
}

void GNet::Location::update( const Address & address , const std::string & name )
{
	G_ASSERT( m_family == AF_UNSPEC || address.domain() == m_family ) ;
	m_address = address ;
	m_family = address.domain() ; // not family()
	m_address_valid = true ;
	m_canonical_name = name ;
	m_update_time = G::DateTime::now() ;
	G_DEBUG( "GNet::Location::ctor: resolved location [" << displayString() << "]" ) ;
}

std::string GNet::Location::name() const
{
	return m_canonical_name ;
}

std::string GNet::Location::displayString() const
{
	const char * ipvx = m_family == AF_UNSPEC ? "ip" : ( m_family == AF_INET ? "ipv4" : "ipv6" ) ;
	return resolved() ? address().displayString() : (m_host+"/"+m_service+"/"+ipvx) ;
}

G::EpochTime GNet::Location::updateTime() const
{
	return m_update_time ;
}

bool GNet::Location::socks() const
{
	return m_socks ;
}

unsigned int GNet::Location::socksFarPort() const
{
	return m_socks_far_port ;
}

std::string GNet::Location::socksFarHost() const
{
	return m_socks_far_host ;
}

/// \file glocation.cpp
