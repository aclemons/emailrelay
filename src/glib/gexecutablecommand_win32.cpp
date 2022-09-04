//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gexecutablecommand_win32.cpp
///

#include "gdef.h"
#include "gexecutablecommand.h"
#include "gstr.h"
#include <stdexcept>
#include <algorithm>
#include <vector>

bool G::ExecutableCommand::osNativelyRunnable() const
{
	std::string type = Str::lower(m_exe.extension()) ;
	return type == "exe" || type == "bat" ;
}

void G::ExecutableCommand::osAddWrapper()
{
	// use "<windows>/system32/cscript.exe" -- perhaps it would be
	// better to do assoc/ftype and add "/H:CScript" if ends up as
	// "wscript.exe" -- but in any case this is only a convenience
	// that the user is free to specify explicitly

	std::string windows ;
	{
		std::vector<char> buffer( MAX_PATH+1 ) ;
		buffer.at(0) = '\0' ;
		unsigned int n = ::GetWindowsDirectoryA( &buffer[0] , MAX_PATH ) ;
		if( n == 0 || n > MAX_PATH )
			throw WindowsError() ;
		windows = std::string( &buffer[0] , n ) ;
	}

	// m_exe=<exe>, m_args=[<arg> ...]
	// m_exe="cscript.exe" m_args=["//nologo" "//B" <exe> <arg> ...]

	std::reverse( m_args.begin() , m_args.end() ) ;
	{
		m_args.push_back( m_exe.str() ) ;
		m_args.push_back( "//B" ) ;
		m_args.push_back( "//nologo" ) ;
	}
	std::reverse( m_args.begin() , m_args.end() ) ;

	m_exe = Path( windows , "system32" , "cscript.exe" ) ;
}

