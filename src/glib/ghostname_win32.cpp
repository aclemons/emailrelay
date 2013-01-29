//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// ghostname_win32.cpp
//

#include "gdef.h"
#include "glimits.h"
#include "ghostname.h"
#include "genvironment.h"
#include "gstr.h"

std::string G::hostname()
{
	char buffer[G::limits::net_hostname] = { '\0' } ;
	if( 0 == ::gethostname( buffer , sizeof(buffer)-1U ) )
	{
		buffer[sizeof(buffer)-1U] = '\0' ;
		return std::string(buffer) ;
	}
	else
	{
		return G::Str::toPrintableAscii( 
			G::Environment::get("COMPUTERNAME",std::string()) , '_' ) ;
	}
}

/// \file ghostname_win32.cpp
