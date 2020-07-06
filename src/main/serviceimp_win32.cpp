//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// serviceimp_win32.cpp
//

#include "gdef.h"
#include "serviceimp.h"
#include "serviceinstall.h"
#include "serviceremove.h"

std::string ServiceImp::install( const std::string & commandline , const std::string & name , const std::string & display_name ,
	const std::string & description )
{
	return service_install( commandline , name , display_name , description , true ) ;
}

std::string ServiceImp::remove( const std::string & service_name )
{
	return service_remove( service_name ) ;
}

std::pair<ServiceImp::ServiceHandle,DWORD> ServiceImp::statusHandle( const std::string & service_name , HandlerFn fn )
{
	DWORD e = 0 ;
	ServiceHandle h = RegisterServiceCtrlHandlerA( service_name.c_str() , fn ) ;
	if( h == 0 )
		e = GetLastError() ;
	return std::make_pair( h , e ) ;
}

DWORD ServiceImp::dispatch( ServiceMainFn fn )
{
	static TCHAR empty[] = { 0 } ;
	static SERVICE_TABLE_ENTRY table [] = { { empty , fn } , { nullptr , nullptr } } ;
	bool ok = !! StartServiceCtrlDispatcher( table ) ; // this doesn't return until the service is stopped
	DWORD e = GetLastError() ;
	return ok ? DWORD(0) : e ;
}

DWORD ServiceImp::setStatus( ServiceHandle hservice , DWORD new_state , DWORD timeout_ms ) noexcept
{
	SERVICE_STATUS s{} ;
	s.dwServiceType = SERVICE_WIN32_OWN_PROCESS ;
	s.dwCurrentState = new_state ;
	s.dwControlsAccepted = SERVICE_ACCEPT_STOP ;
	s.dwWin32ExitCode = NO_ERROR ;
	s.dwServiceSpecificExitCode = 0 ;
	s.dwCheckPoint = 0 ;
	s.dwWaitHint = timeout_ms ;
	bool ok = !!SetServiceStatus( hservice , &s ) ;
	DWORD e = GetLastError() ;
	return ok ? DWORD(0) : e ;
}

void ServiceImp::log( const std::string & ) noexcept
{
	//static std::ofstream f( "c:/temp/temp.out" ) ;
	//f << s << std::endl ;
}

/// \file serviceimp_win32.cpp
