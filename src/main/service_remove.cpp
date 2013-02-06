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
// service_remove.cpp
//

#include "service_remove.h"
#ifdef _WIN32
#include "windows.h"
#include <sstream>

std::string service_remove( const std::string & name )
{
	SC_HANDLE hmanager = OpenSCManager( NULL , NULL , SC_MANAGER_ALL_ACCESS ) ;
	SC_HANDLE hservice = hmanager ? OpenServiceA( hmanager , name.c_str() , DELETE | SERVICE_STOP ) : 0 ;

	// stop it
	SERVICE_STATUS status ;
	bool stop_ok = hservice ? !!ControlService( hservice , SERVICE_CONTROL_STOP , &status ) : false ;
	if( stop_ok )
		Sleep( 1000 ) ; // TODO -- arbitrary sleep to allow the service to stop

	// remove it
	std::string reason ;
	{
		bool delete_ok = hservice ? !!DeleteService( hservice ) : false ;
		if( !delete_ok )
		{
			DWORD e = GetLastError() ;
			std::ostringstream ss ;
			ss << "cannot remove the service \"" << name << "\": " << e ;
			if( e == ERROR_SERVICE_MARKED_FOR_DELETE ) ss << " (already marked for deletion)" ;
			if( e == ERROR_ACCESS_DENIED ) ss << " (access denied)" ;
			if( e == ERROR_SERVICE_DOES_NOT_EXIST ) ss << " (no such service)" ;
			reason = ss.str() ;
		}
	}

	if( hservice ) CloseServiceHandle( hservice ) ;
	if( hmanager ) CloseServiceHandle( hmanager ) ;
	return reason ;
}

#else

std::string service_remove( const std::string & )
{
	return std::string() ;
}

#endif
/// \file service_remove.cpp
