//
// Copyright (C) 2001-2010 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include <stdexcept>

G::Path Dir::windows()
{
	char buffer[MAX_PATH+20U] = { '\0' } ;
	unsigned int n = sizeof(buffer) ;
	::GetWindowsDirectory( buffer , n-1U ) ;
	buffer[n-1U] = '\0' ;
	return G::Path(buffer) ;
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
	return special("programs") + "emailrelay" ; // was windows()
}

G::Path Dir::os_spool() const
{
	return windows() + "spool" + "emailrelay" ;
}

G::Path Dir::os_pid()
{
	return G::Path() ;
}

G::Path Dir::os_pid( const G::Path & , const G::Path & config_dir )
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
	char buffer[10000] = { '\0' } ;
	unsigned long n = ::GetCurrentDirectory( sizeof(buffer)-1U , buffer ) ;
	buffer[sizeof(buffer)-1U] = '\0' ;
	std::string s = n ? std::string(buffer) : std::string(".") ;
	return G::Path( s ) ;
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
	typedef HRESULT (*FnBasic)( HWND , int , HANDLE , DWORD , LPTSTR ) ;
	typedef HRESULT (*FnSpecial)( HWND , LPTSTR , int , BOOL ) ;
}

G::Path Dir::special( const std::string & type )
{
	// use dynamic loading since NT does not have SHGetFolderPath()
	static HMODULE h = LoadLibrary( "SHELL32.DLL" ) ;
	FARPROC fp_basic = h ? GetProcAddress( h , "SHGetFolderPathA" ) : NULL ;
	FARPROC fp_special = h ? GetProcAddress( h , "SHGetSpecialFolderPathA" ) : NULL ;
	if( fp_basic == NULL || fp_special == NULL )
		return ntspecial( type ) ;
	FnBasic fn_basic = reinterpret_cast<FnBasic>(fp_basic) ;
	FnSpecial fn_special = reinterpret_cast<FnSpecial>(fp_special) ;

	if( type == "programs" )
	{
		char buffer[MAX_PATH] = { '\0' } ;
		bool ok = S_OK == (*fn_basic)( NULL , CSIDL_PROGRAM_FILES , NULL , SHGFP_TYPE_CURRENT , buffer ) ;
		buffer[sizeof(buffer)-1U] = '\0' ;
		return ok ? G::Path(std::string(buffer)) : G::Path("c:/program files") ;
	}
	else
	{
		char buffer[MAX_PATH] = { '\0' } ;
		bool ok = NOERROR == (*fn_special)( NULL , buffer , special_id(type) , FALSE ) ;
		buffer[sizeof(buffer)-1U] = '\0' ;
		ok = buffer[0] != '\0' ; // (the return value seems to lie)
		return ok ? G::Path(std::string(buffer)) : windows() ;
	}
}

G::Path Dir::ntspecial( const std::string & type )
{
	std::string user = env("USERNAME") ;
	G::Path user_profile = windows() + "Profiles" + user ;
	G::Path common_profile = windows() + "Profiles" + "All Users" ;

	if( type == "desktop" ) 
	{
		return user_profile + "Desktop" ;
	}
	if( type == "menu" ) 
	{
		return user_profile + "Start Menu" + "Programs" ;
	}
	if( type == "login" ) 
	{
		return user_profile + "Start Menu" + "Programs" + "Startup" ;
	}
	if( type == "programs" )
	{
		G::Path p = env("ProgramFiles") ; // doesnt work on nt :-(
		if( p.str().empty() )
			p = "c:/program files" ;
		return p ;
	}
	return G::Path() ;
}

/// \file dir_win32.cpp
