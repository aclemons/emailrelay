//
// Copyright (C) 2001-2008 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// garg_unix.cpp
//

#include "gdef.h"
#include "garg.h"
#include "gstr.h"
#include "glimits.h"
#include <sstream>
#include <sys/types.h> // pid_t
#include <unistd.h> // getpid(), readlink()

void G::Arg::setExe()
{
	// a better-than-nothing implementation...

	char buffer[limits::path] = { '\0' } ;
	int n = ::readlink( "/proc/self" , buffer , sizeof(buffer) ) ;
	if( n > 0 )
	{
		std::ostringstream ss ; 
		ss << ::getpid() ;
		bool procfs = std::string(buffer,n) == ss.str() ;
		if( procfs )
		{
			n = ::readlink( "/proc/self/exe" , buffer , sizeof(buffer) ) ;
			if( n > 0 )
				m_array[0] = std::string(buffer,n) ;
		}
	}
	else
	{
		// could use getenv("_") on some systems, but too 
		// unreliable in general
	}
}

/// \file garg_unix.cpp
