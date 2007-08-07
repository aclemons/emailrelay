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
// glocal_unix.cpp
//

#include "gdef.h"
#include "glocal.h"
#include "gresolver.h"
#include "gstr.h"
#include "glog.h"
#include <sys/utsname.h>
#include <cstdlib> // getenv

std::string GNet::Local::hostname()
{
	struct utsname info ;
	int rc = ::uname( &info ) ;
	if( rc == -1 ) 
		throw Error("uname") ;

	std::string name = std::string(info.nodename) ;
	std::string::size_type pos = name.find('.') ;
	if( pos != std::string::npos )
		name = name.substr( 0U , pos ) ;

	// pathalogically "uname -n" can be empty, so
	// allow "export HOSTNAME=localhost" as a
	// workround
	//
	if( name.empty() )
	{
		const char * p = std::getenv( "HOSTNAME" ) ;
		name = G::Str::toPrintableAscii( std::string(p?p:"") , '_' ) ;
	}

	return name ;
}

GNet::Address GNet::Local::canonicalAddressImp()
{
	ResolverInfo info( hostname() , "0" ) ;
	std::string error = Resolver::resolve( info ) ;
	if( !error.empty() )
		throw Error(error) ;
	return info.address() ;
}

std::string GNet::Local::fqdnImp()
{
	ResolverInfo info( hostname() , "0" ) ;
	std::string error = Resolver::resolve( info ) ;
	if( !error.empty() )
		throw Error(error) ;
	G_DEBUG( "GNet::Local::fqdnImp: \"" << info.name() << "\"" ) ;
	return info.name() ;
}

/// \file glocal_unix.cpp
