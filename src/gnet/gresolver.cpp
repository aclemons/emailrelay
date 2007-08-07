//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gresolver.cpp
//

#include "gdef.h"
#include "gresolver.h"
#include "gexception.h"
#include "gsocket.h"
#include "gevent.h"
#include "gstr.h"
#include "gdebug.h"
#include "glog.h"

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

std::string GNet::Resolver::resolve( ResolverInfo & in , bool udp )
{
	std::string error ;
	unsigned int port = resolveService( in.service() , udp , error ) ;
	if( ! error.empty() )
	{
		G_DEBUG( "GNet::Resolver::resolve: service error: \"" << in.service() << "\": " << error ) ;
		return error ;
	}

	ResolverInfo result( in ) ; // copy to ensure atomic update
	const bool valid_host = resolveHost( in.host() , port , result ) ;
	if( !valid_host )
	{
		G_DEBUG( "GNet::Resolver::resolve: host error: \"" << in.host() << "\"" ) ;
		return std::string("invalid hostname: \"") + in.host() + "\"" ;
	}
	else
	{
		G_DEBUG( "GNet::Resolver::resolve: \"" << in.host() << "\" + "
			<< "\"" << in.service() << "\" -> "
			<< "\"" << result.address().displayString() << "\" "
			<< "(" << result.name() << ")" ) ;

		in = result ;
		return std::string() ;
	}
}

bool GNet::Resolver::parse( const std::string & s , std::string & host , std::string & service )
{
	std::string::size_type pos = s.rfind( ':' ) ;
	if( pos == std::string::npos || pos == 0U || (pos+1U) == s.length() )
		return false ;
	host = s.substr(0U,pos) ;
	service = s.substr(pos+1U) ;
	return true ;
}

/// \file gresolver.cpp
