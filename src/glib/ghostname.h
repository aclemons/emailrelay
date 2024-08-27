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
/// \file ghostname.h
///

#ifndef G_HOSTNAME_H
#define G_HOSTNAME_H

#include "gdef.h"
#include <string>

namespace G
{
	std::string hostname() ;
		///< Returns the hostname. This may or may not
		///< relate to the host's name on some network.
		///< Returns the empty string on error.
}

#endif

