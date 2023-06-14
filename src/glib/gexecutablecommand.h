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
/// \file gexecutablecommand.h
///

#ifndef G_EXECUTABLE_COMMAND_H
#define G_EXECUTABLE_COMMAND_H

#include "gdef.h"
#include "gpath.h"
#include "gstringarray.h"
#include "gexception.h"
#include "gstringarray.h"
#include <string>

namespace G
{
	class ExecutableCommand ;
}

//| \class G::ExecutableCommand
/// A structure representing an external program, holding a path
/// and a set of arguments. The constructor takes a complete command-line and splits it up
/// into the executable part and a list of command-line parameters. If the
/// command-line starts with a script then the contructed command-line may
/// be for the appropriate interpreter (depending on the o/s).
///
/// \see G::Path, G::Args
///
class G::ExecutableCommand
{
public:
	G_EXCEPTION( WindowsError , tx("cannot determine the windows directory") ) ;

	explicit ExecutableCommand( const std::string & command_line = {} ) ;
		///< Constructor taking a complete command-line. The
		///< command-line is split up on unescaped-and-unquoted
		///< space characters. Uses G::Arg::parse() in its
		///< implementation.

	ExecutableCommand( const G::Path & exe , const StringArray & args ) ;
		///< Constructor taking the executable and arguments
		///< explicitly.

	Path exe() const ;
		///< Returns the executable.

	StringArray args() const ;
		///< Returns the command-line arguments.

	void add( const std::string & arg ) ;
		///< Adds a command-line argument.

	void insert( const G::StringArray & ) ;
		///< Inserts at the front of the command-line.
		///< The first element becomes the new executable.

	std::string displayString() const ;
		///< Returns a printable representation for logging and diagnostics.

private:
	G::Path m_exe ;
	G::StringArray m_args ;
} ;

#endif
