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
// glocal_win32.cpp
//

#include "gdef.h"
#include "glocal.h"
#include "gresolve.h"
#include "glog.h"
#include <sstream>

std::string GNet::Local::hostname()
{
	char buffer[1024U] = { '\0' } ;
	if( 0 != ::gethostname( buffer , sizeof(buffer)-1U ) )
	{
		int error = ::WSAGetLastError() ;
		std::ostringstream ss ;
		ss << "gethostname() (" << error << ")" ;
		throw Error( ss.str() ) ;
	}
	buffer[sizeof(buffer)-1U] = '\0' ;
	return std::string(buffer) ;
}

GNet::Address GNet::Local::canonicalAddressImp()
{
	std::pair<Resolver::HostInfo,std::string> rc = Resolver::resolve( hostname() , "0" ) ;
	if( rc.second.length() != 0U )
	{
		std::ostringstream ss ;
		ss << "resolve: " << rc.second ;
		throw Error( ss.str() ) ;
	}

	return rc.first.address ;
}

std::string GNet::Local::fqdnImp()
{
	std::pair<Resolver::HostInfo,std::string> rc = Resolver::resolve( hostname() , "0" ) ;
	if( rc.second.length() != 0U )
	{
		std::ostringstream ss ;
		ss << "resolve: " << rc.second ;
		throw Error( ss.str() ) ;
	}

	std::string result = rc.first.canonical_name ;

	size_t pos = result.find( '.' ) ;
	if( pos == std::string::npos )
	{
		G_WARNING( "GNet::Local: no valid domain in \"" << result << "\": defaulting to \".local\"" ) ;
		result.append( ".local" ) ;
	}

	return result ;
}

/// \file glocal_win32.cpp
