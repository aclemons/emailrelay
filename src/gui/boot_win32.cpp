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
// boot_win32.cpp
//

#include "gdef.h"
#include "boot.h"
#include "gpath.h"
#include "gmapfile.h"
#include "gfile.h"
#include "service_install.h"
#include "service_remove.h"
#include <stdexcept>

namespace
{
	bool createConfigurationFile( const G::Path & bat , const G::Path & wrapper_exe , bool do_throw )
	{
		G::Path config_file = wrapper_exe.withoutExtension() ;
		std::ofstream file( (config_file.str()+".cfg").c_str() , std::ios_base::out | std::ios_base::trunc ) ;
		G::MapFile::writeItem( file , "dir-config" , bat.dirname().str() ) ;
		file.close() ;
		if( file.fail() && do_throw )
			throw std::runtime_error( "failed to create service wrapper configuration file " + config_file.str() ) ;
		return !file.fail() ;
	}
}

bool Boot::able( const G::Path & )
{
	SC_HANDLE hmanager = OpenSCManager( NULL , NULL , SC_MANAGER_ALL_ACCESS ) ;
	bool ok = hmanager != 0 ;
	if( hmanager ) CloseServiceHandle( hmanager ) ;
	return ok ;
}

bool Boot::install( const G::Path & , const std::string & name , const G::Path & bat , const G::Path & wrapper_exe )
{
	// the 'bat' path is the batch file containing the full command-line
	// for the server process -- the service wrapper knows how to read it
	// at service start time to assemble the full server command-line -- the
	// batch file must be located in a directory given by a configuration
	// file having the same name as the wrapper but with ".exe" replaced
	// by ".cfg" -- for backwards compatibility it can also be located
	// in the same directory as the wrapper

	// install the service
	std::string qwrapper = wrapper_exe.str().find(" ") != std::string::npos ?
		( std::string() + "\"" + wrapper_exe.str() + "\"" ) : wrapper_exe.str() ;
	std::string display_name = "E-MailRelay" ;
	std::string description = display_name + " service (reads " + bat.str() + " at service start time)" ;
	std::string reason = ::service_install( qwrapper , name , display_name , description ) ;
	if( !reason.empty() )
		throw std::runtime_error( reason ) ;

	// create the config file
	createConfigurationFile( bat , wrapper_exe , true/*do_throw*/ ) ;

	return true ;
}

bool Boot::uninstall( const G::Path & , const std::string & name , const G::Path & bat , const G::Path & wrapper_exe )
{
	// create an unused config file -- the user can edit it for a manual service install
	createConfigurationFile( bat , wrapper_exe , false/*do_throw*/ ) ;

	return ::service_remove(name).empty() ;
}

bool Boot::installed( const G::Path & , const std::string & name )
{
	return ::service_installed( name ) ;
}

/// \file boot_win32.cpp
