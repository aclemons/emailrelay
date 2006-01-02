//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// glocal.cpp
//

#include "gdef.h"
#include "glocal.h"
#include "gassert.h"
#include "gdebug.h"
#include <sstream>

std::string GNet::Local::m_fqdn ;
std::string GNet::Local::m_fqdn_override ;
GNet::Address GNet::Local::m_canonical_address(1U) ;

GNet::Address GNet::Local::canonicalAddress()
{
	if( m_canonical_address.port() == 1U )
	{
		m_canonical_address = canonicalAddressImp() ;
		G_ASSERT( m_canonical_address.port() != 1U ) ;
	}
	return m_canonical_address ;
}

std::string GNet::Local::domainname()
{
	std::string full = fqdn() ;
	size_t pos = full.rfind( '.' ) ;
	if( pos == std::string::npos )
		throw Error( "invalid fqdn" ) ;

	G_DEBUG( "GNet::Local::domainname: \"" << full.substr(pos+1U) << "\"" ) ;
	return full.substr( pos+1U ) ;
}

GNet::Address GNet::Local::localhostAddress()
{
	return Address::localhost( 0U ) ;
}

std::string GNet::Local::fqdn()
{
	if( m_fqdn.empty() )
	{
		m_fqdn = m_fqdn_override.empty() ? fqdnImp() : m_fqdn_override ;
	}
	return m_fqdn ;
}

void GNet::Local::fqdn( const std::string & override )
{
	m_fqdn_override = override ;
}

bool GNet::Local::isLocal( const Address & address )
{
	std::string reason ;
	return isLocal( address , reason ) ;
}

bool GNet::Local::isLocal( const Address & address , std::string & reason )
{
	return address.isLocal(reason,canonicalAddress()) ;
}

