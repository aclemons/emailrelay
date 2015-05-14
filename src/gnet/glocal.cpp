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
// glocal.cpp
//

#include "gdef.h"
#include "glocal.h"
#include "ghostname.h"
#include "gassert.h"
#include "gdebug.h"
#include <sstream>

std::string GNet::Local::m_fqdn ;
std::string GNet::Local::m_fqdn_override ;
bool GNet::Local::m_fqdn_override_set = false ;
GNet::Address GNet::Local::m_canonical_address( 0U ) ;
bool GNet::Local::m_canonical_address_set = false ;
bool GNet::Local::m_canonical_address_valid = false ;

std::string GNet::Local::hostname()
{
	std::string name = G::hostname() ;
	return name.empty() ? std::string("localhost") : name ;
}

GNet::Address GNet::Local::canonicalAddress()
{
	if( !m_canonical_address_set )
	{
		m_canonical_address = canonicalAddressImp( GNet::Address(1U) ) ;
		m_canonical_address_valid = m_canonical_address.port() != 1U ;
		m_canonical_address_set = true ;
	}
	return m_canonical_address ;
}

std::string GNet::Local::fqdn()
{
	if( m_fqdn_override_set )
		m_fqdn = m_fqdn_override ;
	else if( m_fqdn.empty() )
		m_fqdn = fqdnImp() ;
	return m_fqdn ;
}

void GNet::Local::fqdn( const std::string & override )
{
	m_fqdn_override = override ;
	m_fqdn_override_set = true ;
}

bool GNet::Local::isLocal( const Address & address )
{
	std::string reason ;
	return isLocal( address , reason ) ;
}

bool GNet::Local::isLocal( const Address & address , std::string & reason )
{
	Address canonical_address = canonicalAddress() ;
	return m_canonical_address_valid ?
		address.isLocal( reason , canonical_address ) :
		address.isLocal( reason ) ;
}

/// \file glocal.cpp
