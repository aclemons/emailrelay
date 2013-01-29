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
// gresolver_dns_disabled.cpp
//

#include "gdef.h"
#include "gresolver.h"
#include "gevent.h"
#include "gstr.h"
#include "gdebug.h"
#include "glog.h"

class GNet::ResolverImp 
{
} ;

// ==

GNet::Resolver::Resolver( EventHandler & )
{
}

GNet::Resolver::~Resolver()
{
}

bool GNet::Resolver::resolveReq( std::string , bool )
{
	return false ;
}

bool GNet::Resolver::resolveReq( std::string , std::string , bool )
{
	return false ;
}

void GNet::Resolver::resolveCon( bool , const Address & , std::string )
{
}

bool GNet::Resolver::busy() const
{
	return false ;
}

unsigned int GNet::Resolver::resolveService( const std::string & , bool , std::string & error )
{
	error = "invalid service name: dns lookup disabled: use a port number" ;
	return 0U ;
}

std::string GNet::Resolver::resolveHost( const std::string & host_name , unsigned int port , ResolverInfo & info )
{
	try
	{
		info.update( Address(host_name,port) , host_name ) ;
		return std::string() ;
	}
	catch( G::Exception & ) // Address::BadString
	{
		return std::string("invalid hostname \"") + host_name + "\": dns lookup disabled: use an ip address" ;
	}
}

/// \file gresolver_dns_disabled.cpp
