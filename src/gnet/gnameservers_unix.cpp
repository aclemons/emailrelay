//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gnameservers_unix.cpp
///

#include "gdef.h"
#include "gnameservers.h"
#include "gstr.h"
#include "gstringview.h"
#include "gstringtoken.h"
#include <fstream>

std::vector<GNet::Address> GNet::nameservers( unsigned int port )
{
	std::vector<GNet::Address> result ;
	std::string line ;
	std::ifstream f( "/etc/resolv.conf" ) ;
	while( G::Str::readLine( f , line ) )
	{
		G::string_view sv( line ) ;
		G::StringTokenView t( sv , " \t" ) ;
		if( t && G::Str::imatch(t(),"nameserver") )
		{
			if( ++t && GNet::Address::validStrings(G::sv_to_string(t()),"0") )
				result.push_back( GNet::Address::parse( G::sv_to_string(t()) , port ) ) ;
		}
	}
	return result ;
}

