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
// glocal_dns_disabled.cpp
//

#include "gdef.h"
#include "glocal.h"
#include "ghostname.h"

std::string GNet::Local::m_name_override ;
bool GNet::Local::m_name_override_set = false ;

std::string GNet::Local::hostname()
{
	std::string name = G::hostname() ;
	if( name.empty() )
		throw Error("cannot determine the hostname for this machine" ) ;
	return name ;
}

std::string GNet::Local::canonicalName()
{
	return m_name_override_set ? m_name_override : hostname() ;
}

void GNet::Local::canonicalName( const std::string & override )
{
	m_name_override = override ;
	m_name_override_set = true ;
}

bool GNet::Local::isLocal( const Address & address , std::string & reason )
{
	bool local = address.sameHost( Address::loopback() ) ;
}

/// \file glocal_dns_disabled.cpp
