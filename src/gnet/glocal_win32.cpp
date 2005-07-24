//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
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

std::string GNet::Local::hostname()
{
	char buffer[1024U] ;
	if( 0 != ::gethostname( buffer , sizeof(buffer)-1U ) )
	{
		int error = ::WSAGetLastError() ;
		throw Error( std::ostringstream() << "gethostname() (" << error << ")" ) ;
	}
	return std::string(buffer) ;
}

GNet::Address GNet::Local::canonicalAddress()
{
	static bool first = true ; // avoid multiple synchronous DNS queries
	static Address result( Address::invalidAddress() ) ;
	if( first )
	{
		first = false ;

		std::pair<Resolver::HostInfo,std::string> rc = 
			Resolver::resolve( hostname() , "0" ) ;

		if( rc.second.length() != 0U )
			throw Error( std::ostringstream() << "resolve: " << rc.second ) ;

		result = rc.first.address ;
	}
	return result ;
}

std::string GNet::Local::fqdnImp()
{
	static bool first = true ; // avoid multiple synchronous DNS queries
	static std::string result ;
	if( first )
	{
		first = false ;

		std::pair<Resolver::HostInfo,std::string> rc = 
			Resolver::resolve( hostname() , "0" ) ;

		if( rc.second.length() != 0U )
			throw Error( std::ostringstream() << "resolve: " << rc.second ) ;

		result = rc.first.canonical_name ;

		size_t pos = result.find( '.' ) ;
		if( pos == std::string::npos )
		{
			G_WARNING( "GNet::Local: no valid domain in \"" << result << "\": defaulting to \".local\"" ) ;
			result.append( ".local" ) ;
		}
	}
	return result ;
}

