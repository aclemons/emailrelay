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
//
// gdaemon_unix.cpp
//

#include "gdef.h"
#include "gdaemon.h"
#include "gprocess.h"
#include "gnewprocess.h"

void G::Daemon::detach( PidFile & pid_file )
{
	pid_file.check() ; // absolute path
	detach() ;
}

void G::Daemon::detach()
{
	// see Stevens, ISBN 0-201-563137-7, ch 13.

	if( NewProcess::fork() == NewProcess::Parent )
		::_exit( 0 ) ;

	setsid() ;
	G_IGNORE_RETURN(bool) Process::cd( "/" , Process::NoThrow() ) ;

	if( NewProcess::fork() == NewProcess::Parent )
		::_exit( 0 ) ;
}

void G::Daemon::setsid()
{
	G_IGNORE_RETURN(pid_t) ::setsid() ;
}

/// \file gdaemon_unix.cpp
