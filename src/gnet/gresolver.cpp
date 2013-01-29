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
// gresolver.cpp
//

#include "gdef.h"
#include "gresolver.h"
#include "gstr.h"
#include "gdebug.h"
#include "glog.h"

std::string GNet::Resolver::resolve( ResolverInfo & in , bool udp )
{
	// service

	unsigned int port = 0U ;
	std::string service_name = in.service() ;
	if( !service_name.empty() && G::Str::isNumeric(service_name) )
	{
		if( ! G::Str::isUInt(service_name) )
			return "silly port number" ;

		port = G::Str::toUInt(service_name) ;
		if( ! Address::validPort(port) )
			return "invalid port number" ;
	}
	else
	{
		std::string error ;
		port = resolveService( in.service() , udp , error ) ;
		if( !error.empty() )
			return error ;
	}

	// host

	ResolverInfo result( in ) ; // copy to ensure atomic update
	std::string host_error = resolveHost( in.host() , port , result ) ;
	if( !host_error.empty() )
		return host_error ;

	in = result ;
	return std::string() ;
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
