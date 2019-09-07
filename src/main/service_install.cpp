//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#include "service_remove.h"
#include <windows.h>
#include <sstream>
#include <utility>

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

namespace
{
	struct Result
	{
		bool manager ;
		DWORD e ;
		std::string reason ;
		Result() :
			manager(false) ,
			e(0)
		{
		}
		Result( bool manager_ , DWORD e_ ) :
			manager(manager_) ,
			e(e_)
		{
			if( manager )
				reason = std::string() + "cannot attach to the service manager (" + decode(e) + ")" ;
			else
				reason = std::string() + "cannot create the service (" + decode(e) + ")" ;
		}
	} ;
}

static Result install( std::string commandline , std::string name , std::string display_name ,
	std::string description , bool autostart )
{
	SC_HANDLE hmanager = OpenSCManager( nullptr , nullptr , SC_MANAGER_ALL_ACCESS ) ;
	if( hmanager == 0 )
	{
		DWORD e = GetLastError() ;
		return Result(true,e) ;
	}

	DWORD start_type = autostart ? SERVICE_AUTO_START : SERVICE_DEMAND_START ;

	SC_HANDLE hservice = CreateServiceA( hmanager , name.c_str() , display_name.c_str() ,
		SERVICE_ALL_ACCESS , SERVICE_WIN32_OWN_PROCESS , start_type , SERVICE_ERROR_NORMAL ,
		commandline.c_str() ,
		nullptr , nullptr , nullptr , nullptr , nullptr ) ;

	if( hservice == 0 )
	{
		DWORD e = GetLastError() ;
		CloseServiceHandle( hmanager ) ;
		return Result(false,e) ;
	}

	if( description.empty() )
		description = ( display_name + " service" ) ;

	if( REG_SZ > 5 && (description.length()+5) > REG_SZ )
	{
		description.resize(REG_SZ-5) ;
		description.append( "..." ) ;
	}

	SERVICE_DESCRIPTIONA service_description ;
	service_description.lpDescription = const_cast<char*>(description.c_str()) ;
	ChangeServiceConfig2A( hservice , SERVICE_CONFIG_DESCRIPTION , &service_description ) ; // ignore errors

	CloseServiceHandle( hservice ) ;
	CloseServiceHandle( hmanager ) ;
	return Result() ;
}

std::string service_install( std::string commandline , std::string name , std::string display_name ,
	std::string description , bool autostart )
{
	if( name.empty() || display_name.empty() )
		return "invalid zero-length service name" ;

	Result r = install( commandline , name , display_name , description , autostart ) ;
	if( !r.manager && r.e == ERROR_SERVICE_EXISTS )
	{
		std::string error = service_remove( name ) ;
		if( !error.empty() )
			return error ;

		r = install( commandline , name , display_name , description , autostart ) ;
	}

	return r.reason ;
}

bool service_installed( std::string name )
{
	SC_HANDLE hmanager = OpenSCManager( nullptr , nullptr , SC_MANAGER_CONNECT ) ;
	if( hmanager == 0 )
		return false ;

	SC_HANDLE hservice = OpenService( hmanager , name.c_str() , SERVICE_QUERY_CONFIG ) ;
	if( hservice == 0 )
	{
		CloseServiceHandle( hmanager ) ;
		return false ;
	}

	CloseServiceHandle( hservice ) ;
	CloseServiceHandle( hmanager ) ;
	return true ;
}

#else

std::string service_install( std::string , std::string , std::string , std::string , bool )
{
	return std::string() ;
}

bool service_installed( std::string )
{
	return false ;
}

#endif
/// \file service_install.cpp
