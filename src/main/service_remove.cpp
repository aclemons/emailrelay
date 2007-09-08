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
// service_remove.cpp
//

#include "service_remove.h"

#ifdef _WIN32

#include "windows.h"

std::string service_remove( const std::string & name )
{
	SC_HANDLE hmanager = OpenSCManager( NULL , NULL , SC_MANAGER_ALL_ACCESS ) ;
	SC_HANDLE hservice = hmanager ? OpenService( hmanager , name.c_str() , DELETE ) : 0 ;
	bool ok = hservice ? !!DeleteService( hservice ) : false ;
	if( hservice ) CloseServiceHandle( hservice ) ;
	if( hmanager ) CloseServiceHandle( hmanager ) ;
	return ok ? std::string() : ( std::string() + "cannot remove the service \"" + name + "\"" ) ;
}

#else

std::string service_remove( const std::string & name )
{
	return std::string() ;
}

#endif

/// \file service_remove.cpp
