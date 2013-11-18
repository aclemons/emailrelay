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
// dir_win32.cpp
//

#include "gdef.h"
#include "gconvert.h"
#include <shlwapi.h>
#include <shlobj.h>

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

std::string Dir::dotexe()
{
	return ".exe" ;
}

G::Path Dir::os_install()
{
	return special("programs") + "emailrelay" ;
}

G::Path Dir::os_gui( const G::Path & base )
{
	return base + "emailrelay-gui.exe" ;
}

G::Path Dir::os_icon( const G::Path & base )
{
	return os_server( base ) ; // icon is a resource in the exe
}

G::Path Dir::os_server( const G::Path & base )
{
	return base + "emailrelay.exe" ;
}

G::Path Dir::os_config()
{
	return special("programs") + "emailrelay" ;
}

G::Path Dir::os_spool()
{
	return windows() + "system32" + "spool" + "emailrelay" ;
}

G::Path Dir::os_pid( const G::Path & config_dir )
{
	return config_dir ;
}

G::Path Dir::os_boot()
{
	// empty implies no access (see call to Boot::able() in pages.cpp),
	// so the default has to be a bogus value
	return "services" ;
}

G::Path Dir::os_bootcopy( const G::Path & , const G::Path & )
{
	return G::Path() ;
}

G::Path Dir::cwd()
{
	DWORD n = ::GetCurrentDirectoryA( 0 , NULL ) ;
	char * buffer = new char [n+2U] ;
	buffer[0] = '\0' ;
	n = ::GetCurrentDirectoryA( n+1U , buffer ) ;
	std::string dir( buffer , n ) ;
	delete [] buffer ;
	if( n == 0U )
		throw std::runtime_error( "cannot determine the current working directory" ) ;
	return G::Path( dir ) ;
}

namespace
{
	int special_id( const std::string & type )
	{
		if( type == "desktop" ) return CSIDL_DESKTOPDIRECTORY ;
		if( type == "menu" ) return CSIDL_PROGRAMS ;
		if( type == "login" ) return CSIDL_STARTUP ;
		throw std::runtime_error("internal error") ;
		return 0 ;
	}
}

G::Path Dir::special( const std::string & type )
{
	if( type == "programs" )
	{
		char buffer[MAX_PATH] = { 0 } ;
		bool ok = S_OK == ::SHGetFolderPathA( NULL , CSIDL_PROGRAM_FILES , NULL , SHGFP_TYPE_CURRENT , buffer ) ;
		return ok ? G::Path(buffer) : G::Path("c:/program files") ;
	}
	else
	{
		char buffer[MAX_PATH] = { 0 } ;
		bool ok = TRUE == ::SHGetSpecialFolderPathA( NULL , buffer , special_id(type) , FALSE ) ;
		std::string result = ok ? std::string(buffer) : std::string() ;
		G::Path default_ = envPath("USERPROFILE",envPath("HOME")) ;
		return result.empty() ? default_ : G::Path(result) ;
	}
}

G::Path Dir::home()
{
	return envPath( "USERPROFILE" , envPath( "HOME" , desktop() ) ) ;
}

/// \file dir_win32.cpp
