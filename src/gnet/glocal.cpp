//
// Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gdebug.h"

std::string GNet::Local::m_fqdn_override ;

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
	return 
		m_fqdn_override.empty() ?
			fqdnImp() :
			m_fqdn_override ;
}

void GNet::Local::fqdn( const std::string & override )
{
	m_fqdn_override = override ;
}

