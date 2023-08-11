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
/// \file ghostname_win32.cpp
///

#include "gdef.h"
#include "ghostname.h"
#include "genvironment.h"
#include "gstr.h"

std::string G::hostname()
{
	std::vector<char> buffer( 257U , '\0' ) ; // documented hostname limit of 256
	if( 0 == ::gethostname( &buffer[0] , static_cast<int>(buffer.size()-1U) ) && buffer.at(0U) != '\0' )
	{
		buffer[buffer.size()-1U] = '\0' ;
		return std::string( &buffer[0] ) ;
	}
	else
	{
		return Str::printable( Environment::get("COMPUTERNAME",std::string()) , '_' ) ;
	}
}

