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

namespace G
{
	class Daemon ;
}

//| \class G::Daemon
/// A static interface for daemonising the calling process. Daemonisation
/// includes fork()ing, detaching from the controlling terminal, setting
/// the process umask, etc. The windows implementation does nothing.
/// \see G::Process
///
class G::Daemon
{
public:
	static void detach() ;
		///< Detaches from the parent environment. This typically
		///< involves fork()ing, std::_Exit()ing the parent, and calling
		///< setsid() in the child. See also G::PidFile.

	static void detach( const G::Path & pid_file ) ;
		///< Does a detach() but the calling process waits a while
		///< for the pid file to be created before it exits.

public:
	Daemon() = delete ;

private:
	static void setsid() ;
} ;

#endif
