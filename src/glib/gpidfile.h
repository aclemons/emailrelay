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
/// \file gpidfile.h
///

#ifndef G_PIDFILE_H
#define G_PIDFILE_H

#include "gdef.h"
#include "gexception.h"
#include "gsignalsafe.h"
#include "gpath.h"
#include <sys/types.h>
#include <string>

/// \namespace G
namespace G
{
	class PidFile ;
	class Daemon ;
}

/// \class G::PidFile
/// A class for creating pid files. Works 
/// with G::Root and G::Daemon so that the pid file can 
/// get created very late in a daemon startup sequence. 
/// Installs a signal handler so that the pid file gets
/// deleted on process termination.
///
/// Usage:
/// \code
/// G::PidFile pid_file ;
/// if( !path.empty() ) pid_file.init(path) ;
/// G::Root::init("nobody") ;
/// if( daemon ) G::Daemon::detach( pid_file ) ;
/// { G::Root claim_root ; pid_file.commit() ; }
/// \endcode
///
/// \see G::Daemon
///
class G::PidFile 
{
public:
	G_EXCEPTION( Error , "invalid pid file" ) ;

	static void cleanup( SignalSafe , const char * path ) ;
		///< Deletes the specified pid file if it
		///< contains this process's id.
		///<
		///< Signal-safe, reentrant implementation.

	explicit PidFile( const Path & pid_file_path ) ;
		///< Constructor. The path should normally be
		///< an absolute path. Use commit() to actually
		///< create the file.

	PidFile() ;
		///< Default constructor. Constructs a
		///< do-nothing object. Initialise with init().

	void init( const Path & pid_file_path ) ;
		///< Used after default construction.

	~PidFile() ;
		///< Destructor. Calls cleanup() to delete
		///< the file.

	void commit() ;
		///< Creates the file.

	void check() ;
		///< Throws an exception if the path is not
		///< absolute.

	Path path() const ;
		///< Returns the path as supplied to the constructor
		///< or init().

private:
	Path m_path ;

private:
	PidFile( const PidFile & ) ; // not implemented
	void operator=( const PidFile & ) ; // not implemented
	static bool mine( SignalSafe , const char * path ) ; // reentrant
	static void create( const Path & pid_file ) ;
	bool valid() const ;
} ;

#endif

