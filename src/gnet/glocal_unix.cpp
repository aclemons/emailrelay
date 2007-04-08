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
// glocal_unix.cpp
//

#include "gdef.h"
#include "glocal.h"
#include "gresolve.h"
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
	size_t pos = name.find('.') ;
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
	std::pair<Resolver::HostInfo,std::string> rc = Resolver::resolve( hostname() , "0" ) ;
	if( rc.second.length() != 0U )
		throw Error(rc.second) ;

	return rc.first.address ;
}

std::string GNet::Local::fqdnImp()
{
	std::pair<Resolver::HostInfo,std::string> rc = Resolver::resolve( hostname() , "0" ) ;
	if( rc.second.length() != 0U )
		throw Error(rc.second) ;

	G_DEBUG( "GNet::Local::fqdnImp: \"" << rc.first.canonical_name << "\"" ) ;
	return rc.first.canonical_name ;
}

/// \file glocal_unix.cpp
