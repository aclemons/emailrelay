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
// gresolve_ipv6.cpp
//

#include "gdef.h"
#include "gresolve.h"
#include "gdebug.h"
#include "glog.h"

#if defined( AI_DEFAULT )
// RFC 2553

//static
bool GNet::Resolver::resolveHost( const std::string & host_name , HostInfo & host_info )
{
	hostent * host = NULL ;
	try
	{
		int error = 0 ;
		host = ::getipnodebyname( host_name.c_str() , AF_INET6 , AI_DEFAULT , &error ) ;
		if( host != NULL )
		{
			const char * h_name = host->h_name ;
			host_info.canonical_name = std::string(h_name?h_name:"") ;
			host_info.address = Address( *host , 0U ) ;
			::freehostent( host ) ;
		}
		return host != NULL ;
	}
	catch(...)
	{
		if( host != NULL ) ::freehostent( host ) ;
		throw ;
	}
}

#else
// RFC 2133 (obsolete)

#include <resolv.h> // requires -D_USE_BSD
extern "C" { struct hostent * gethostbyname2( const char * , int ) ; } ;

//static
bool GNet::Resolver::resolveHost( const std::string & host_name , HostInfo & host_info )
{
	res_init() ;
	_res.options |= RES_USE_INET6 ;

	hostent * host = ::gethostbyname2( host_name.c_str() , AF_INET6 ) ;
	if( host != NULL )
	{
G_DEBUG( "GNet::Resolver::resolveHost: canonical name of \"" << host_name << "\" is \"" << host->h_name << "\"" ) ;
		host_info.canonical_name = std::string(host->h_name) ;
		host_info.address = Address( *host , 0U ) ;
	}
	return host != NULL ;
}

#endif

