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
/// \file gexecutable.h
///
	
#ifndef G_EXE_H
#define G_EXE_H

#include "gdef.h"
#include "gpath.h"
#include "gstrings.h"
#include <string>

/// \namespace G
namespace G
{
	class Executable ;
}

/// \class G::Executable
/// A structure representing an external program,
/// holding a path and a set of arguments. The constructor
/// takes a complete command-line and splits it up into the
/// executable part and a list of command-line parameters.
///
/// \see G::Path, G::Args
///
class G::Executable 
{
public:
	explicit Executable( const std::string & command_line = std::string() ) ;
		///< Constructor taking a complete command-line.
		///< The command-line is split up on unescaped
		///< space characters.

	Path exe() const ;
		///< Returns the executable.

	Strings args() const ;
		///< Returns the command-line arguments.

	std::string displayString() const ;
		///< Returns a printable representation for logging and diagnostics.

private:
	bool osNativelyRunnable() const ;
	void osAddWrapper() ;

private:
	G::Path m_exe ;
	G::Strings m_args ;
} ;

#endif

