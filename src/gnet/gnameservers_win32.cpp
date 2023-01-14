//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
///
/// \file gnameservers_win32.cpp
///

#include "gdef.h"
#include "gnameservers.h"
#include "gstr.h"
#include "gstringview.h"
#include "gstringtoken.h"
#include "gscope.h"
#include "glog.h"
#include <iphlpapi.h>
#include <fstream>
#include <cstdlib>

std::vector<GNet::Address> GNet::nameservers( unsigned int port )
{
	std::vector<GNet::Address> result ;

	void * info_buffer = std::malloc( sizeof(FIXED_INFO) ) ;
	FIXED_INFO * info = static_cast<FIXED_INFO*>(info_buffer) ;
	ULONG size = sizeof(FIXED_INFO) ;
	G::ScopeExit _( [&info_buffer](){if(info_buffer) std::free(info_buffer);} ) ;

	auto rc = info_buffer ? GetNetworkParams( info , &size ) : ERROR_NO_DATA ;
	if( rc == ERROR_BUFFER_OVERFLOW )
	{
		info_buffer = std::realloc( info_buffer , size == ULONG(0) ? ULONG(1) : size ) ;
		info = static_cast<FIXED_INFO*>(info_buffer) ;
		if( info_buffer != nullptr )
			rc = GetNetworkParams( info , &size ) ;
	}
	if( rc == NO_ERROR )
	{
		const char * p = info->DnsServerList.IpAddress.String ;
		if( GNet::Address::validStrings( p?p:"" , "0" ) )
			result.push_back( GNet::Address::parse( p , port ) ) ;

		for( const IP_ADDR_STRING * addr = info->DnsServerList.Next ; addr ; addr = addr->Next )
		{
			p = addr->IpAddress.String ;
			if( GNet::Address::validStrings( p?p:"" , "0" ) )
				result.push_back( GNet::Address::parse( p , port ) ) ;
		}
	}
	return result ;
}

