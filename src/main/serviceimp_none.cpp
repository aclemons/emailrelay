//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
///
/// \file serviceimp_none.cpp
///
// A do-nothing implementation that might be useful when testing the
// service wrapper without a service manager, eg. with Wine.
//

#include "gdef.h"
#include "serviceimp.h"
#include "servicecontrol.h"
#include <iostream>

namespace ServiceImp
{
	std::string name_ ;
	constexpr DWORD SERVICE_STOPPED = 1 ; // see winsvc.h
	constexpr DWORD SERVICE_START_PENDING = 2 ;
	constexpr DWORD SERVICE_STOP_PENDING = 3 ;
	constexpr DWORD SERVICE_RUNNING = 4 ;
}

std::string ServiceImp::install( const std::string & , const std::string & name , const std::string & ,
	const std::string & )
{
	std::cout << "ServiceImp::install: " << name << std::endl ;
	name_ = name ;
	return {} ;
}

std::string ServiceImp::remove( const std::string & name )
{
	std::cout << "ServiceImp::remove: " << name << std::endl ;
	return {} ;
}

std::pair<ServiceImp::StatusHandle,DWORD> ServiceImp::statusHandle( const std::string & , HandlerFn )
{
	StatusHandle h = 1 ;
	return { h , 0 } ;
}

DWORD ServiceImp::dispatch( ServiceMainFn fn )
{
	sleep( 1 ) ;
	fn( 1 , name_.c_str() ) ;
	while( true )
		sleep( 1 ) ;
	return 0 ;
}

DWORD ServiceImp::setStatus( StatusHandle , DWORD new_state , DWORD ) noexcept
{
	std::string s ;
	if( new_state == SERVICE_STOPPED ) s = "stopped" ;
	if( new_state == SERVICE_START_PENDING ) s = "start-pending" ;
	if( new_state == SERVICE_STOP_PENDING ) s = "stop-pending" ;
	if( new_state == SERVICE_RUNNING ) s = "running" ;
	std::cout << "ServiceImp::setStatus: " << new_state << " " << s << std::endl ;
	return 0 ;
}

void ServiceImp::log( const std::string & s ) noexcept
{
	std::cout << s << std::endl ;
}

