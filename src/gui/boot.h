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
///
/// \file boot.h
///

#ifndef G_GUI_BOOT_H
#define G_GUI_BOOT_H

#include "gdef.h"
#include "gpath.h"
#include "gstrings.h"

/// \class Boot
/// Provides support for installing as a boot-time service.
///
class Boot
{
public:
	static bool able( const G::Path & dir_boot ) ;
		///< Returns true if the operating-system is supported and the supplied
		///< boot-system directory is valid. The parameter normally comes
		///< from Dir::boot().

	static bool install( const G::Path & dir_boot , const std::string & name ,
		const G::Path & path_1 , const G::Path & path_2 ) ;
			///< Installs the target as a boot-time service. For Windows path_1 is
			///< the batch file and path_2 is the service wrapper. For Unix path_1
			///< is the startstop script and path_2 is the server executable.

	static bool uninstall( const G::Path & dir_boot , const std::string & name ,
		const G::Path & path_1 , const G::Path & path_2 ) ;
			///< Uninstalls the target as a boot-time service.

	static bool installed( const G::Path & dir_boot , const std::string & name ) ;
		///< Returns true if currently installed.

private:
	Boot() ;
} ;

#endif

