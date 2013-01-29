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
	static bool able( G::Path dir_boot ) ;
		///< Returns true if the operating-system is supported
		///< and the supplied boot-system directory is valid.
		///< The parameter normally comes from Dir::boot().

	static bool install( G::Path dir_boot , G::Path target , G::Strings args ) ;
		///< Installs the target as a boot-time service.

	static bool uninstall( G::Path dir_boot , G::Path target , G::Strings args ) ;
		///< Uninstalls the target as a boot-time service.

private:
	Boot() ;
} ;

#endif

