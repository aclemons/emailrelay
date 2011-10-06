//
// Copyright (C) 2001-2011 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gmessagestore_win32.cpp
//

#include "gdef.h"
#include "glimits.h"
#include "gsmtp.h"
#include "gmessagestore.h"
#include "gpath.h"

G::Path GSmtp::MessageStore::defaultDirectory()
{
	char buffer[G::limits::path] ;
	if( 0 == ::GetWindowsDirectory( buffer , sizeof(buffer)-1U ) )
		buffer[0] = '\0' ;

	G::Path path( buffer ) ;
	path.pathAppend( "spool" ) ;
	path.pathAppend( "emailrelay" ) ;
	return path ;
}

/// \file gmessagestore_win32.cpp
