//
// Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gpidfile.h
//

#ifndef G_PIDFILE_H
#define G_PIDFILE_H

#include "gdef.h"
#include "gexception.h"
#include "gpath.h"
#include <sys/types.h>
#include <string>

namespace G
{
	class PidFile ;
	class Daemon ;
}

// Class: G::PidFile
// Description: A class for creating pid files.
// See also: G::Daemon
//
class G::PidFile 
{
public:
	G_EXCEPTION( Error , "invalid pid file" ) ;

	static void cleanup( const char * path ) ;
		// Deletes the specified pid file if it
		// contains this process's id.
		//
		// Reentrant implementation.

	explicit PidFile( const Path & pid_file_path ) ;
		// Constructor. The path should normally be
		// an absolute path. Use commit() to actually
		// create the file.

	PidFile() ;
		// Default constructor. Constructs a
		// do-nothing object. Initialise with init().

	void init( const Path & pid_file_path ) ;
		// Used after default construction.

	~PidFile() ;
		// Destructor. Calls cleanup() to delete
		// the file.

	void commit() ;
		// Creates the file.

	void check() ;
		// Throws an exception if the path is not
		// absolute.

	Path path() const ;
		// Returns the path as supplied to the constructor
		// or init().

private:
	Path m_path ;

private:
	PidFile( const PidFile & ) ; // not implemented
	void operator=( const PidFile & ) ; // not implemented
	static bool mine( const char * path ) ; // reentrant
	static void create( const Path & pid_file ) ;
	bool valid() const ;
} ;

#endif

