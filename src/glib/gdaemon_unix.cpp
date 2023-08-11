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
/// \file gdaemon_unix.cpp
///

#include "gdef.h"
#include "gdaemon.h"
#include "gprocess.h"
#include "gfile.h"
#include "gnewprocess.h"
#include <chrono>
#include <thread>

namespace G
{
	namespace DaemonImp
	{
		void waitfor( const G::Path & pid_file )
		{
			if( !pid_file.empty() )
			{
				for( int i = 0 ; i < 100 ; i++ )
				{
					if( G::File::exists( pid_file , std::nothrow ) )
						break ;
					std::this_thread::sleep_for( std::chrono::milliseconds(10) ) ;
				}
			}
		}
	}
}

#ifndef G_LIB_SMALL
void G::Daemon::detach()
{
	detach( G::Path() ) ;
}
#endif

void G::Daemon::detach( const G::Path & pid_file )
{
	// see Stevens, ISBN 0-201-563137-7, ch 13.

	if( !NewProcess::fork().first )
	{
		DaemonImp::waitfor( pid_file ) ; // because systemd
		std::_Exit( 0 ) ; // exit from parent
	}

	setsid() ;
	Process::cd( "/" , std::nothrow ) ;

	if( !NewProcess::fork().first )
		std::_Exit( 0 ) ; // exit from parent
}

void G::Daemon::setsid()
{
	GDEF_IGNORE_RETURN ::setsid() ;
}

