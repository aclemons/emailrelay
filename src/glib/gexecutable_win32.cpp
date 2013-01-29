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
// gexecutable_win32.cpp
//

#include "gdef.h"
#include "gexecutable.h"
#include "glimits.h"
#include "gstr.h"
#include <stdexcept>

bool G::Executable::osNativelyRunnable() const
{
	std::string type = G::Str::lower(m_exe.extension()) ;
	return type == "exe" || type == "bat" ;
}

void G::Executable::osAddWrapper()
{
	std::string windows ;
	{
    	char buffer[MAX_PATH] = { 0 } ;
    	unsigned int n = ::GetWindowsDirectoryA( buffer , MAX_PATH ) ;
    	if( n == 0 || n > MAX_PATH )
        	throw std::runtime_error( "cannot determine the windows directory" ) ;
    	windows = std::string( buffer , n ) ;
	}

	G::Path cscript( windows , "system32" , "cscript.exe" ) ;
	m_args.push_front( m_exe.str() ) ;
	m_args.push_front( "//B" ) ; // portable?
	m_args.push_front( "//nologo" ) ;
	m_exe = cscript ;
}

/// \file gexecutable_win32.cpp
