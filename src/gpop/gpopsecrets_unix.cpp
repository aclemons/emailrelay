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
// gpopsecrets_unix.cpp
//

#include "gdef.h"
#include "gpop.h"
#include "gpopsecrets.h"
#include "gpath.h"
#include "gmacros.h"

#ifndef G_SYSCONFDIR
#define G_SYSCONFDIR
#endif

std::string GPop::Secrets::defaultPath()
{
	std::string sysconfdir( G_STR(G_SYSCONFDIR) ) ;
	if( sysconfdir.empty() )
		sysconfdir = "/etc" ;

	G::Path path( sysconfdir ) ;
	path.pathAppend( "emailrelay.auth" ) ;
	return path.str() ;
}

/// \file gpopsecrets_unix.cpp
