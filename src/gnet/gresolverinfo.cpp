//
// Copyright (C) 2001-2010 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gresolverinfo.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gresolverinfo.h"
#include "gresolver.h"
#include "gassert.h"

std::string GNet::ResolverInfo::part( const std::string & s , bool first )
{
	std::string host_part ;
	std::string service_part ;
	if( !Resolver::parse(s,host_part,service_part) )
		throw InvalidFormat( s ) ;
	return first ? host_part : service_part ;
}

GNet::ResolverInfo::ResolverInfo( const std::string & host_and_service ) :
	m_host(part(host_and_service,true)) ,
	m_service(part(host_and_service,false)) ,
	m_address_valid(false) ,
	m_address(Address::invalidAddress()) ,
	m_update_time(0U)
{
}

GNet::ResolverInfo::ResolverInfo( const char * host_and_service ) :
	m_host(part(host_and_service,true)) ,
	m_service(part(host_and_service,false)) ,
	m_address_valid(false) ,
	m_address(Address::invalidAddress()) ,
	m_update_time(0U)
{
}

GNet::ResolverInfo::ResolverInfo( const std::string & host , const std::string & service ) :
	m_host(host) ,
	m_service(service) ,
	m_address_valid(false) ,
	m_address(Address::invalidAddress()) ,
	m_update_time(0U)
{
}

std::string GNet::ResolverInfo::host() const
{
	return m_host ;
}

std::string GNet::ResolverInfo::service() const
{
	return m_service ;
}

bool GNet::ResolverInfo::hasAddress() const
{
	return m_address_valid ;
}

GNet::Address GNet::ResolverInfo::address() const
{
	return m_address ;
}

void GNet::ResolverInfo::update( const Address & address , const std::string & name )
{
	m_address = address ;
	m_address_valid = true ;
	m_canonical_name = name ;
	m_update_time = G::DateTime::now() ;
}

std::string GNet::ResolverInfo::name() const
{
	return m_canonical_name ;
}

std::string GNet::ResolverInfo::str() const
{
	return m_host + ":" + m_service ;
}

std::string GNet::ResolverInfo::displayString( bool simple ) const
{
	std::string s = m_host + ":" + m_service ;
	if( simple && hasAddress() )
	{
		s = address().displayString() ;
	}
	else
	{
		if( hasAddress() )
			s.append( std::string() + " [" + address().displayString() + "]" ) ;
		if( !name().empty() )
			s.append( std::string() + " (" + name() + ")" ) ;
	}
	return s ;
}

G::DateTime::EpochTime GNet::ResolverInfo::updateTime() const
{
	return m_update_time ;
}

/// \file gresolverinfo.cpp
