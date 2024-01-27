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
/// \file gpidfile.h
///

#ifndef G_PIDFILE_H
#define G_PIDFILE_H

#include "gdef.h"
#include "gexception.h"
#include "gsignalsafe.h"
#include "gprocess.h"
#include "gpath.h"
#include <sys/types.h>
#include <string>

namespace G
{
	class PidFile ;
	class Daemon ;
}

//| \class G::PidFile
/// A class for creating pid files. Works with G::Root and G::Daemon so that
/// the pid file can get created very late in a daemon startup sequence.
/// Installs a signal handler so that the pid file gets deleted on process
/// termination.
///
/// Usage:
/// \code
/// G::PidFile pid_file ;
/// if( !path.empty() )
/// { pid_file.init(path) ; pid_file.check() ; }
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
	G_EXCEPTION( Error , tx("invalid pid file") ) ;

	static bool cleanup( SignalSafe , const char * path ) noexcept ;
		///< Deletes the specified pid file if it contains this
		///< process's id. Claims root privilege to read and
		///< delete the pid file (see G::Root::atExit()).
		///<
		///< This is not normally needed by client code since it
		///< is installed as a signal handler (see G::Cleanup)
		///< and called from the destructor.
		///<
		///< Signal-safe, reentrant implementation.

	explicit PidFile( const Path & pid_file_path ) ;
		///< Constructor. A relative path is converted to
		///< an absolute path using the cwd. Use commit()
		///< to actually create the file.

	PidFile() ;
		///< Default constructor. Constructs a do-nothing
		///< object.

	~PidFile() ;
		///< Destructor. Calls cleanup() to delete the file.

	void mkdir() ;
		///< Creates the directory if it does not already exist.
		///<
		///< The caller should switch effective user-id and
		///< umask as necessary.

	void commit() ;
		///< Creates the pid file if a path has been defined.
		///< Also installs signal handlers to cleanup() the
		///< file on abnormal process termination. Throws
		///< on error.
		///<
		///< The caller should switch effective user-id and
		///< umask as necessary.

	bool committed() const ;
		///< Returns true if commit() has been called with
		///< a valid path().

	Path path() const ;
		///< Returns the full path of the file.

public:
	PidFile( const PidFile & ) = delete ;
	PidFile( PidFile && ) = delete ;
	PidFile & operator=( const PidFile & ) = delete ;
	PidFile & operator=( PidFile && ) = delete ;

private:
	static void create( const Path & pid_file ) ;
	static Process::Id read( SignalSafe , const char * path ) noexcept ;
	bool valid() const ;

private:
	Path m_path ;
	bool m_committed{false} ;
} ;

#endif
