//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// gsystem.h
//

#ifndef G_SYSTEM_H
#define G_SYSTEM_H

#include "gpath.h"

// Class: GSystem
// Description: Provides file-system paths.
//
class GSystem 
{
public:
	static G::Path install() ;
		// Returns the installation path.

	static G::Path spool() ;
		// Returns the spool directory path.

	static G::Path config() ;
		// Returns the configuration directory path.

	static G::Path cwd() ;
		// Returns the current working directory.

private:
	GSystem() ;
} ;

#endif

