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
/// \file guiaccess.h
///

#ifndef GUI_INSTALLER_ACCESS_H
#define GUI_INSTALLER_ACCESS_H

#include "gdef.h"
#include "gpath.h"
#include "gexception.h"
#include "gstringarray.h"
#include <string>

namespace Gui
{
	class Access ;
}

//| \class Gui::Access
/// A static class for modifying file-system permissions.
///
class Gui::Access
{
public:
	static bool modify( const G::Path & , bool ) ;
		///< Modifies the permissions on the given path in
		///< some undefined way. Returns false on error.

public:
	Access() = delete ;
} ;

#endif
