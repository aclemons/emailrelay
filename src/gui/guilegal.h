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
/// \file guilegal.h
///

#ifndef GUI_LEGAL_H
#define GUI_LEGAL_H

#include "gdef.h"
#include <string>
#include <vector>

namespace Gui
{
	class Legal ;
}

//| \class Gui::Legal
/// A static class providing warranty and copyright text.
///
class Gui::Legal
{
public:
	static const char * text() ;
		///< Returns the introductory legal text.

	static const char * license() ;
		///< Returns the license text.

	static std::vector<std::string> credits() ;
		///< Returns the third-party library credits.

public:
	Legal() = delete ;
} ;

#endif
