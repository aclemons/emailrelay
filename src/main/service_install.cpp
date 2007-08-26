//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// service_install.cpp
//

#include "service_install.h"

#ifdef _WIN32

#include <windows.h>
#include <sstream>

static std::string decode( DWORD e )
{
	switch( e )
	{
		case ERROR_ACCESS_DENIED: return "access denied" ;
		case ERROR_DATABASE_DOES_NOT_EXIST: return "service database does not exist" ;
		case ERROR_INVALID_PARAMETER: return "invalid parameter" ;
		case ERROR_CIRCULAR_DEPENDENCY: return "circular dependency" ;
		case ERROR_DUPLICATE_SERVICE_NAME: return "duplicate service name" ;
		case ERROR_INVALID_HANDLE: return "invalid handle" ;
		case ERROR_INVALID_NAME: return "invalid name" ;
		case ERROR_INVALID_SERVICE_ACCOUNT: return "invalid service account" ;
		case ERROR_SERVICE_EXISTS: return "service already exists" ;
	}
	std::ostringstream ss ;
	ss << e ;
	return ss.str() ;
}

std::string service_install( std::string commandline , std::string name , std::string display_name )
{
	if( name.empty() || display_name.empty() )
		return "invalid zero-length service name" ;

	SC_HANDLE hmanager = OpenSCManager( NULL , NULL , SC_MANAGER_ALL_ACCESS ) ;
	if( hmanager == 0 )
	{
		DWORD e = GetLastError() ;
		return std::string() + "cannot attach to the service manager (" + decode(e) + ")" ;
	}

	SC_HANDLE hservice = CreateService( hmanager , name.c_str() , display_name.c_str() ,
		SERVICE_ALL_ACCESS , SERVICE_WIN32_OWN_PROCESS , SERVICE_AUTO_START , SERVICE_ERROR_NORMAL ,
		commandline.c_str() , 
		NULL , NULL , NULL , NULL , NULL ) ;

	if( hservice == 0 )
	{
		DWORD e = GetLastError() ;
		return std::string() + "cannot create the service (" + decode(e) + ")" ;
	}
		
	CloseServiceHandle( hservice ) ;
	CloseServiceHandle( hmanager ) ;
	return std::string() ;
}

#else

std::string service_install( std::string , std::string , std::string )
{
	return std::string() ;
}

#endif
/// \file service_install.cpp
