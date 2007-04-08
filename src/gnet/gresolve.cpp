//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gresolve.cpp
//

#include "gdef.h"
#include "gresolve.h"
#include "gexception.h"
#include "gsocket.h"
#include "gevent.h"
#include "gstr.h"
#include "gdebug.h"
#include "glog.h"

//static
unsigned int GNet::Resolver::resolveService( const std::string & service_name , bool udp , std::string & error )
{
	if( service_name.length() != 0U && G::Str::isNumeric(service_name) )
	{
		if( ! G::Str::isUInt(service_name) )
		{
			error = "silly port number" ;
			return 0U ;
		}

		unsigned int port = G::Str::toUInt(service_name) ;
		if( ! Address::validPort(port) )
		{
			error = "invalid port number" ;
			return 0U ;
		}

		return port ;
	}
	else
	{
		servent * service = ::getservbyname( service_name.c_str() , udp ? "udp" : "tcp" ) ;
		if( service == NULL )
		{
			error = "invalid service name" ;
			return 0U ;
		}
		Address service_address( *service ) ;
		return service_address.port() ;
	}
}

//static 
GNet::Resolver::HostInfoPair GNet::Resolver::resolve( const std::string & host_name ,
	const std::string & service_name , bool udp )
{
	HostInfo host_info ;
	const bool valid_host = resolveHost( host_name.c_str() , host_info ) ;
	if( !valid_host )
	{
		G_DEBUG( "GNet::Resolver::resolve: host error: \"" << host_name << "\"" ) ;
		return HostInfoPair( HostInfo() , std::string("invalid hostname: \"" + host_name + "\"" ) ) ;
	}

	std::string error ;
	unsigned int port = resolveService( service_name , udp , error ) ;
	if( ! error.empty() )
	{
		G_DEBUG( "GNet::Resolver::resolve: service error: \"" << service_name << "\": " << error ) ;
		return HostInfoPair( HostInfo() , error ) ;
	}

	host_info.address.setPort( port ) ;

	G_DEBUG( "GNet::Resolver::resolve: \"" << host_name << "\" + "
		<< "\"" << service_name << "\" -> "
		<< "\"" << host_info.address.displayString() << "\" "
		<< "(" << host_info.canonical_name << ")" ) ;

	return HostInfoPair( host_info , std::string() ) ;
}

// ===

GNet::Resolver::HostInfo::HostInfo() :
	address( Address::invalidAddress() )
{
}

/// \file gresolve.cpp
