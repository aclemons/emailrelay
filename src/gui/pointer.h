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
/// \file pointer.h
///

#ifndef G_GUI_POINTER_H
#define G_GUI_POINTER_H

#include "gdef.h"
#include "gpath.h"
#include "gstrings.h"
#include <map>
#include <string>

/// \class Pointer
/// Provides access to the pointer file (typically created by
/// "make install").
///
class Pointer 
{
public:

	static G::Path file( const std::string & argv0 ) ;
		///< Returns the name of the pointer file associated with the given executable.

	static void read( G::StringMap & map , std::istream & ) ;
		///< Reads variables from a pointer file.

	static void write( std::ostream & , const G::StringMap & map , const G::Path & executable ) ;
		///< Writes a complete pointer file to the given stream.
		///< Adds a hash-bang line at the beginning and an "exec" line 
		///< at the end so that the pointer file can act as a shell
		///< script that runs the specified executable.

private:
	Pointer() ;
} ;

#endif

