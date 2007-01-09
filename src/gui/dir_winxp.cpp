//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// dir_winxp.cpp
//

#define _WIN32_IE 0x600
#include <shlwapi.h>
#include <shlobj.h>
#ifndef SHGFP_TYPE_CURRENT
#define SHGFP_TYPE_CURRENT 0
#endif

#include "dir.h"
#include "gpath.h"
#include <stdexcept>

namespace
{
	int special_id( const std::string & type )
	{
		if( type == "desktop" ) return CSIDL_DESKTOPDIRECTORY ;
		if( type == "menu" ) return CSIDL_PROGRAMS ;
		if( type == "login" ) return CSIDL_STARTUP ;
		if( type == "lib" ) return CSIDL_APPDATA ;
		throw std::runtime_error("internal error") ;
		return 0 ;
	}
}

G::Path Dir::special( const std::string & type )
{
	if( type == "programs" )
	{
		char buffer[MAX_PATH] = { '\0' } ;
		bool ok = S_OK == SHGetFolderPath( NULL , CSIDL_PROGRAM_FILES , NULL , SHGFP_TYPE_CURRENT , buffer ) ;
		buffer[sizeof(buffer)-1U] = '\0' ;
		return ok ? G::Path(std::string(buffer)) : G::Path("c:/program files") ;
	}
	else if( type == "reskit" )
	{
		return special("programs")+"resource kit";
	}
	else
	{
		char buffer[MAX_PATH] = { '\0' } ;
		bool ok = NOERROR == SHGetSpecialFolderPath( NULL , buffer , special_id(type) , FALSE ) ;
		buffer[sizeof(buffer)-1U] = '\0' ;
		ok = buffer[0] != '\0' ; // (the return value seems to lie)
		return ok ? G::Path(std::string(buffer)) : windows() ;
	}
}

