//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file options.h
///

#ifndef G_MAIN_OPTIONS_H
#define G_MAIN_OPTIONS_H

#include "gdef.h"
#include "goptions.h"
#include <string>

namespace Main
{
	class Options ;
}

//| \class Main::Options
/// Provides the emailrelay command-line options specification string.
/// \see G::OptionParser
///
class Main::Options
{
public:
	static G::Options spec( bool is_windows ) ;
		///< Returns an o/s-specific G::OptionParser specification.

public:
	Options() = delete ;
} ;

#endif
