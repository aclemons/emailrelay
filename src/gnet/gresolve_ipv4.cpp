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
// gresolve_ipv4.cpp
//

#include "gdef.h"
#include "gresolve.h"

//static
bool GNet::Resolver::resolveHost( const std::string & host_name , HostInfo & host_info )
{
	hostent * host = ::gethostbyname( host_name.c_str() ) ;
	if( host != NULL )
	{
		const char * h_name = host->h_name ;
		host_info.canonical_name = std::string(h_name?h_name:"") ;
		host_info.address = Address( *host , 0U ) ;
	}
	return host != NULL ;
}

