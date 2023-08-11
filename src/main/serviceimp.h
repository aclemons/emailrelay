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
/// \file serviceimp.h
///

#ifndef G_MAIN_SERVICE_IMP_H
#define G_MAIN_SERVICE_IMP_H

#include "gdef.h"
#include <string>

#ifdef G_UNIX
using DWORD = g_uint32_t ;
using SERVICE_STATUS_HANDLE = int ;
using LPTSTR = const char ;
#define WINAPI
#endif

namespace ServiceImp /// A interface used by the service wrapper to talk to the windows service manager.
{
	using StatusHandle = SERVICE_STATUS_HANDLE ;
	using HandlerFn = void (WINAPI *)( DWORD ) ;
	using ServiceMainFn = void (WINAPI *)( DWORD , LPTSTR * ) ;

	std::string install( const std::string & commandline , const std::string & name , const std::string & display_name ,
		const std::string & description ) ;
			///< Installs the service. Returns an error string.

	std::string remove( const std::string & service_name ) ;
		///< Uninstalls the service. Returns an error string.

	std::pair<StatusHandle,DWORD> statusHandle( const std::string & service_name , HandlerFn ) ;
		///< Returns a service handle associated with the given control
		///< callback function.

	DWORD dispatch( ServiceMainFn ) ;
		///< Dispatches messages from the service sub-system to exported
		///< handler functions, although in practice only to the given
		///< ServiceMain function. Only returns when the service stops.

	DWORD setStatus( StatusHandle hservice , DWORD new_state , DWORD timeout_ms ) noexcept ;
		///< Sets the service status. Returns zero on success or
		///< an error number.

	void log( const std::string & ) noexcept ;
		///< Does service-wrapper logging.
}

#endif
