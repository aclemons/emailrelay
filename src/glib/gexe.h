//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gexe.h
//
	
#ifndef G_EXE_H
#define G_EXE_H

#include "gdef.h"
#include "gpath.h"
#include "gstrings.h"
#include <string>

namespace G
{
	class Executable ;
}

// Class: G::Executable
// Description: A structure representing an external program,
// holding a path and a set of arguments.
// See also: G::Path, G::Args
//
class G::Executable 
{
public:
	explicit Executable( const std::string & command_line = std::string() ) ;
		// Constructor taking a complete command-line.
		// The command-line is split up on unescaped
		// space characters.

	explicit Executable( const G::Path & exe ) ;
		// Constructor for an executable with no extra arguments.

	Path exe() const ;
		// Returns the executable.

	Strings args() const ;
		// Returns the command-line arguments.

private:
	G::Path m_exe ;
	G::Strings m_args ;
} ;

#endif

