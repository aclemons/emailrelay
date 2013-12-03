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
// gresolver_ipv4.cpp
//

#include "gdef.h"
#include "gresolver.h"

unsigned int GNet::Resolver::resolveService( const std::string & service_name , bool udp , std::string & error )
{
	servent * service = ::getservbyname( service_name.c_str() , udp ? "udp" : "tcp" ) ;
	if( service == NULL )
	{
		error = "invalid service name" ;
		return 0U ;
	}
	else
	{
		Address service_address( *service ) ;
		return service_address.port() ;
	}
}

std::string GNet::Resolver::resolveHost( const std::string & host_name , unsigned int port , ResolverInfo & result )
{
	hostent * host = ::gethostbyname( host_name.c_str() ) ;
	if( host == NULL )
		return std::string("no such host: \"") + host_name + "\"" ;

	const char * h_name = host->h_name ;
	result.update( Address(*host,port) , std::string(h_name?h_name:"") ) ;
	return std::string() ;
}

/// \file gresolver_ipv4.cpp
