//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// dir_win32.cpp
//

#define _WIN32_IE 0x600
#include <shlwapi.h>
#include <shlobj.h>
#ifndef SHGFP_TYPE_CURRENT
#define SHGFP_TYPE_CURRENT 0
#endif

#include "dir.h"
#include "gfile.h"
#include "gpath.h"
#include <stdexcept>
#include <cstdlib> //getenv

std::string Dir::env( const std::string & key , const std::string & default_ )
{
	const char * p = getenv( key.c_str() ) ;
	if( p == NULL )
		return default_ ;
	return std::string(p) ;
}

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

G::Path Dir::install()
{
	return prefix(special("programs")) + "emailrelay" ;
}

G::Path Dir::config()
{
	return prefix(windows()) ;
}

G::Path Dir::spool()
{
	return prefix(windows()) + "spool" + "emailrelay" ;
}

G::Path Dir::cwd()
{
	char buffer[10000] = { '\0' } ;
	unsigned long n = ::GetCurrentDirectory( sizeof(buffer)-1U , buffer ) ;
	buffer[sizeof(buffer)-1U] = '\0' ;
	std::string s = n ? std::string(buffer) : std::string(".") ;
	return G::Path( s ) ;
}

G::Path Dir::tooldir()
{
	return thisdir() ;
}

G::Path Dir::startup()
{
	return prefix(special("login")) ;
}

G::Path Dir::pid()
{
	return windows() ;
}

G::Path Dir::boot()
{
	return windows() ; // not used
}

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
	else if( type == "reskit" )
	{
		return special("programs")+"resource kit";
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
	if( type == "lib" ) 
	{
		return G::Path() ;
	}
	if( type == "programs" )
	{
		G::Path p = env("ProgramFiles") ; // doesnt work on nt :-(
		if( p.str().empty() )
			p = "c:/program files" ;
		return p ;
	}
	if( type == "reskit" )
	{
		G::Path p = env("NTRESKIT") ;
		if( p.str().empty() )
			p = special("programs")+"resource kit";
		return p ;
	}
	return G::Path() ;
}


/// \file dir_win32.cpp
