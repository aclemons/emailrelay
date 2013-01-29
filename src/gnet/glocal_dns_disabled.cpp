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
// glocal_dns_disabled.cpp
//

#include "gdef.h"
#include "glocal.h"
#include "genvironment.h"

GNet::Address GNet::Local::canonicalAddressImp()
{
	return Address::localhost() ;
}

std::string GNet::Local::fqdnImp()
{
	std::string host = hostname() ;
	if( host.find('.') == std::string::npos )
	{
		std::string domain = G::Environment::get( "DOMAINNAME" , "local" ) ;
		return host + "." + domain ;
	}
	else
	{
		return host ;
	}
}

/// \file glocal_dns_disabled.cpp
