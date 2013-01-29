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
/// \file gdaemon.h
///

#ifndef G_DAEMON_H
#define G_DAEMON_H

#include "gdef.h"
#include "gexception.h"
#include "gpidfile.h"
#include "gpath.h"
#include <sys/types.h>
#include <string>

/// \namespace G
namespace G
{
	class Daemon ;
}

/// \class G::Daemon
/// A class for deamonising the calling process.
/// Deamonisation includes fork()ing, detaching from the
/// controlling terminal, setting the process umask, etc.
/// The windows implementation does nothing.
/// \see G::Process
///
class G::Daemon 
{
public:
	G_EXCEPTION( CannotFork , "cannot fork" ) ;

	static void detach() ;
		///< Detaches from the parent environment.
		///< This typically involves fork()ing,
		///< _exit()ing the parent, and calling 
		///< setsid() in the child.

	static void detach( PidFile & pid_file ) ;
		///< An overload which allows for a delayed write
		///< of the new process-id to a file.
		///<
		///< A delayed write is useful for network daemons
		///< which open a listening port. A second instance
		///< of the process will fail on startup, and should 
		///< not overwrite the pid file of the running
		///< server. In this situation PidFile::commit() 
		///< should be called just before entering the event 
		///< loop.
		///<
		///< Throws PidFile::Error on error.

private:
	Daemon() ;
	static void setsid() ;
} ;

#endif

