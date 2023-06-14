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
/// \file glocal.cpp
///

#include "gdef.h"
#include "glocal.h"
#include "ghostname.h"
#include "gresolver.h"

std::string GNet::Local::hostname()
{
	std::string name = G::hostname() ;
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
		static Location location( hostname().append(":0") ) ;
		bool ok = Resolver::resolve(location).empty() && !location.name().empty() ;
		result = ok ? location.name() : (hostname()+".localnet") ;
	}
	return result ;
}

