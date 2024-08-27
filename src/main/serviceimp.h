//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gstringarray.h"
#include <string>
#include <utility>
#include <functional>

#ifdef G_UNIX
using SERVICE_STATUS_HANDLE = int ;
#endif

namespace ServiceImp /// A interface used by the service wrapper to talk to the windows service manager.
{
	using StatusHandle = SERVICE_STATUS_HANDLE ;
	using HandlerFn = std::function<void(DWORD)> ;
	using ServiceMainFn = std::function<void(G::StringArray)> ;

	std::pair<std::string,DWORD> install( const std::string & commandline , const std::string & name ,
		const std::string & display_name , const std::string & description ) ;
			///< Installs the service. Returns the empty string on
			///< success or a failure reason and GetLastError() value.

	std::pair<std::string,DWORD> remove( const std::string & service_name ) ;
		///< Uninstalls the service. Returns the empty string on
		///< success or a failure reason and GetLastError() value.

	std::pair<StatusHandle,DWORD> statusHandle( const std::string & service_name , HandlerFn ) ;
		///< Returns a service handle associated with the given control
		///< callback function. Returns a non-zero handle on success
		///< or a zero handle with a GetLastError() error value.

	DWORD dispatch( ServiceMainFn ) ;
		///< Dispatches messages from the service sub-system to the given
		///< ServiceMain function. Returns a GetLastError() value.
		///< Only returns when the service stops.

	DWORD setStatus( StatusHandle hservice , DWORD new_state , DWORD timeout_ms ,
		DWORD generic_error = 0 , DWORD specific_error = 0 ) noexcept ;
			///< Sets the service status. Returns zero on success or a
			///< GetLastError() generic error value. If the generic error
			///< is ERROR_SERVICE_SPECIFIC_ERROR then the specific error
			///< is also recorded.

	void log( const std::string & ) noexcept ;
		///< Does service-wrapper logging.
}

#endif
