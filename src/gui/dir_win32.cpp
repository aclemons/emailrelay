//
// Copyright (C) 2001-2015 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// dir_win32.cpp
//

#include "gdef.h"
#include "gconvert.h"
#include <shlwapi.h>
#include <shlobj.h>
#include <vector>

#ifndef SHGFP_TYPE_CURRENT
#define SHGFP_TYPE_CURRENT 0
#endif
#ifndef CSIDL_PROGRAM_FILES
#define CSIDL_PROGRAM_FILES 38
#endif

#include "dir.h"
#include "gfile.h"
#include "gpath.h"
#include "glog.h"
#include <stdexcept>

G::Path Dir::windows()
{
	char buffer[MAX_PATH] = { 0 } ;
	unsigned int n = ::GetWindowsDirectoryA( buffer , MAX_PATH ) ;
	if( n == 0 || n > MAX_PATH )
		throw std::runtime_error( "cannot determine the windows directory" ) ;
	return G::Path( std::string(buffer,n) ) ;
}

G::Path Dir::os_install()
{
	return special("programs") + "emailrelay" ;
}

G::Path Dir::os_config()
{
	return special("data") + "emailrelay" ;
}

G::Path Dir::os_spool()
{
	return special("data") + "emailrelay" + "spool" ;
}

G::Path Dir::os_pid( const G::Path & config_dir )
{
	return special("data") + "emailrelay" ;
}

G::Path Dir::os_boot()
{
	// empty implies no access (see call to Boot::able() in pages.cpp),
	// so the default has to be a bogus value
	return "services" ;
}

namespace
{
	int special_id( const std::string & type )
	{
		if( type == "desktop" ) return CSIDL_DESKTOPDIRECTORY ; // "c:/documents and settings/<username>/desktop"
		if( type == "menu" ) return CSIDL_PROGRAMS ; // "c:/documents and settings/<username>/start menu/programs"
		if( type == "login" ) return CSIDL_STARTUP ; // "c:/documents and settings/<username>/start menu/programs/startup"
		if( type == "programs" ) return CSIDL_PROGRAM_FILES ; // "c:/program files"
		if( type == "data" ) return CSIDL_COMMON_APPDATA ; // "c:/documents and settints/all users/application data"
		throw std::runtime_error("internal error") ;
		return 0 ;
	}
}

G::Path Dir::special( const std::string & type )
{
	char buffer[MAX_PATH] = { 0 } ;
	bool ok = S_OK == ::SHGetFolderPathA( NULL , special_id(type) , NULL , SHGFP_TYPE_CURRENT , buffer ) ;
	return ok ? G::Path(buffer) : G::Path("c:/") ;
}

G::Path Dir::home()
{
	return envPath( "USERPROFILE" , envPath( "HOME" , desktop() ) ) ;
}

G::Path Dir::os_absolute( const G::Path & dir )
{
	G::Path result = dir ;
	if( dir != G::Path() )
	{
		char small_buffer[2U] = { '\0' } ;
		DWORD rc = ::GetFullPathNameA( dir.str().c_str() , 1U , small_buffer , NULL ) ;
		if( rc != 0U )
		{
			std::vector<char> buffer( static_cast<size_t>(rc) ) ;
			rc = ::GetFullPathNameA( dir.str().c_str() , buffer.size() , &buffer[0] , NULL ) ;
			if( rc != 0U )
				result = G::Path( std::string(&buffer[0],static_cast<size_t>(rc) ) ) ;
		}
	}
	return result ;
}

/// \file dir_win32.cpp
