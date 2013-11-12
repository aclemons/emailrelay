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
// glocal_win32.cpp
//

#include "gdef.h"
#include "glimits.h"
#include "glocal.h"
#include "gresolver.h"
#include "glog.h"

GNet::Address GNet::Local::canonicalAddressImp()
{
	ResolverInfo info( hostname() , "0" ) ;
	std::string error = Resolver::resolve( info ) ;
	if( !error.empty() )
	{
		throw Error( std::string() + "resolve: " + error ) ;
	}
	return info.address() ;
}

std::string GNet::Local::fqdnImp()
{
	ResolverInfo info( hostname() , "0" ) ;
	std::string error = Resolver::resolve( info ) ;
	if( !error.empty() )
	{
		throw Error( std::string() + "resolve: " + error ) ;
	}

	std::string result = info.name() ;

	std::string::size_type pos = result.find( '.' ) ;
	if( pos == std::string::npos )
	{
		G_WARNING( "GNet::Local: no domain name in \"" << result << "\": defaulting to \".local\"" ) ;
		result.append( ".local" ) ;
	}

	return result ;
}

/// \file glocal_win32.cpp
