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
// gresolver_ipv6.cpp
//
// Implements resolveHost() using one of:
// - getaddrinfo() [POSIX] [RFC-2553] (TO DO)
// - getipnodebyname() [RFC-2553]
// - gethostbyname2() [RFC-2133] (obsoleted)
//

#include "gdef.h"
#include "gresolver.h"
#include "gdebug.h"
#include "glog.h"

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

#if defined(HAVE_GETIPNODEBYNAME) && HAVE_GETIPNODEBYNAME

std::string GNet::Resolver::resolveHost( const std::string & host_name , unsigned int port , ResolverInfo & result )
{
	hostent * host = NULL ;
	try
	{
		int error = 0 ;
		host = ::getipnodebyname( host_name.c_str() , AF_INET6 , AI_DEFAULT , &error ) ;
		if( host != NULL )
		{
			const char * h_name = host->h_name ;
			result.update( Address(*host,port) , std::string(h_name?h_name:"") ) ;
			::freehostent( host ) ;
		}
		return host == NULL ? ( std::string("no such host: \"") + host_name + "\"" ) : std::string() ;
	}
	catch(...) // rethrown
	{
		if( host != NULL ) ::freehostent( host ) ;
		throw ;
	}
}

#else

#include <resolv.h> // requires -D_USE_BSD
extern "C" { struct hostent * gethostbyname2( const char * , int ) ; } ;

std::string GNet::Resolver::resolveHost( const std::string & host_name , unsigned int port , ResolverInfo & result )
{
	res_init() ;
	_res.options |= RES_USE_INET6 ;

	hostent * host = ::gethostbyname2( host_name.c_str() , AF_INET6 ) ;
	if( host != NULL )
	{
		result.update( Address(*host,port) , std::string(host->h_name) ) ;
	}
	return host == NULL ? ( std::string("no such host: \"") + host_name + "\"" ) : std::string() ;
}

#endif

/// \file gresolver_ipv6.cpp
