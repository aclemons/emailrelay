//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file glocal_win32.cpp
///

#include "gdef.h"
#include "glocal.h"
#include "ghostname.h"
#include "gidn.h"
#include "gnowide.h"

std::string GNet::Local::hostname()
{
	std::string name = G::hostname() ; // ie. GetComputerNameEx(ComputerNamePhysicalDnsHostname)
	if( name.empty() )
		return "localhost" ;
	return name ;
}

std::string GNet::Local::canonicalName()
{
	static std::string result ;
	static bool first = true ;
	if( first )
	{
		first = false ;
		result = G::Idn::encode( G::nowide::getComputerNameEx( ComputerNameDnsFullyQualified ) ) ;
		if( result.empty() )
			result = G::Idn::encode(hostname()) + ".localnet" ;
	}
	return result ;
}

