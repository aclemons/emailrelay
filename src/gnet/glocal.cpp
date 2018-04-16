//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// glocal.cpp
//

#include "gdef.h"
#include "glocal.h"
#include "ghostname.h"
#include "gresolver.h"
#include "gassert.h"
#include "gdebug.h"
#include <sstream>

std::string GNet::Local::m_name_override ;
bool GNet::Local::m_name_override_set = false ;

std::string GNet::Local::hostname()
{
	std::string name = G::hostname() ;
	if( name.empty() )
		return "localhost" ;
	return name ;
}

std::string GNet::Local::resolvedHostname()
{
	static std::string result ;
	static bool first = true ;
	if( first )
	{
		first = false ;
		static Location location( hostname() , "0" ) ;
		bool ok = Resolver::resolve(location).empty() && !location.name().empty() ;
		result = ok ? location.name() : (hostname()+".localnet") ;
	}
	return result ;
}

std::string GNet::Local::canonicalName()
{
	return m_name_override_set ? m_name_override : resolvedHostname() ;
}

void GNet::Local::canonicalName( const std::string & name_override )
{
	m_name_override = name_override ;
	m_name_override_set = true ;
}

bool GNet::Local::isLocal( const Address & address , std::string & reason )
{
	// TODO use getifaddrs(3) and GetAdaptersAddresses()

	// local if a loopback address
	if( address.isLoopback() )
		return true ;

	// ipv6 is easier - no need to use dns
	if( address.family() == Address::Family::ipv6() )
		return address.isLocal( reason ) ;

	// look up and cache the hostname() ipv4 addresses
	typedef std::vector<Address> List ;
	static List list ;
	static bool done = false ;
	if( !done )
	{
		list = Resolver::resolve( hostname() , "0" , AF_INET ) ;
		done = true ;
	}

	// local if a hostname() address
	for( List::iterator p = list.begin() ; p != list.end() ; ++p )
	{
		if( (*p).sameHostPart(address) )
			return true ;
	}

	// format a reason string
	std::stringstream ss ;
	if( list.empty() )
	{
		ss << address.hostPartString() << " is not a loopback address" ;
	}
	else
	{
		ss << address.hostPartString() << " is not a loopback address or " ;
		const char * sep = "" ;
		for( List::iterator p = list.begin() ; p != list.end() ; ++p , sep = " or " )
		{
			ss << sep << (*p).hostPartString() ;
		}
	}
	reason = ss.str() ;

	return false ;
}

/// \file glocal.cpp
