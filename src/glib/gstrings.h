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
/// \file gstrings.h
///

#ifndef G_STRINGS_H
#define G_STRINGS_H

#include "gdef.h"
#include <string>
#include <vector>
#include <map>

namespace G
{
	typedef std::vector<std::string> StringArray ; ///< A std::vector of std::strings.
	typedef std::map<std::string,std::string> StringMap ; ///< A std::map of std::strings.
}

#endif
