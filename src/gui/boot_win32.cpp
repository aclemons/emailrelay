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
// boot_win32.cpp
//

#include "gdef.h"
#include "boot.h"
#include "gpath.h"
#include "gfile.h"
#include "service_install.h"
#include "service_remove.h"
#include <stdexcept>

bool Boot::able( G::Path )
{
	SC_HANDLE hmanager = OpenSCManager( NULL , NULL , SC_MANAGER_ALL_ACCESS ) ;
	bool ok = hmanager != 0 ;
	if( hmanager ) CloseServiceHandle( hmanager ) ;
	return ok ;
}

bool Boot::install( G::Path , G::Path target , G::Strings )
{
	// the supplied 'target' is actually a batch file containing the full
	// command-line for the server process (see installer.cpp) -- the
	// service wrapper knows where to look for the batch file and how to
	// read it to get the full server process command-line

	std::string path = G::Path(target.dirname(),"emailrelay-service.exe").str() ;
	std::string commandline = path.find(" ") != std::string::npos ? ( std::string() + "\"" + path + "\"" ) : path ;
	std::string display_name = "E-MailRelay" ;
	std::string description = display_name + " service (reads " + target.str() + " at service start time)" ;
	std::string reason = service_install( commandline , "emailrelay" , display_name , description ) ;
	if( !reason.empty() )
		throw std::runtime_error( reason ) ;
	return true ;
}

bool Boot::uninstall( G::Path , G::Path , G::Strings )
{
	return service_remove("emailrelay").empty() ;
}

/// \file boot_win32.cpp
