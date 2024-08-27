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
/// \file output.h
///

#ifndef G_MAIN_OUTPUT_H
#define G_MAIN_OUTPUT_H

#include "gdef.h"
#include "goptionsusage.h"
#include <string>

namespace Main
{
	class Output ;
}

//| \class Main::Output
/// An abstract interface for generating output on a command-line
/// or a GUI. The appropriate implementation is selected from
/// main() or WinMain().
///
class Main::Output
{
public:
	virtual void output( const std::string & , bool is_error , bool ) = 0 ;
		///< Outputs the given string.

	virtual G::OptionsUsage::Config outputLayout( bool verbose ) const = 0 ;
		///< Returns a layout definition for G::Options.

	virtual bool outputSimple() const = 0 ;
		///< Returns true if the output is just sent to stdout;
		///< returns false for a fancy gui message box.

	virtual ~Output() = default ;
		///< Destructor.
} ;

#endif
