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
// garg_win32.cpp
//

#include "gdef.h"
#include "garg.h"
#include "glimits.h"
#include "gstr.h"
#include "gdebug.h"
#include <vector>
#include <algorithm>

void G::Arg::setExe()
{
	if( m_array.empty() )
		m_array.push_back( moduleName(NULL) ) ;
	else
		m_array[0] = moduleName( NULL ) ;
}

std::string G::Arg::moduleName( HINSTANCE hinstance )
{
	std::vector<char> buffer( MAX_PATH , '\0' ) ; // was limits::path
	DWORD size = static_cast<DWORD>( buffer.size() ) ;
	DWORD rc = ::GetModuleFileNameA( hinstance , &buffer[0] , size ) ;
	if( rc == 0 ) // some doubt about what's in rc - just test for zero
		return std::string() ; // moot
	if( std::find(buffer.begin(),buffer.end(),'\0') == buffer.end() )
		*buffer.rbegin() = '\0' ;
	return std::string( &buffer[0] ) ;
}

void G::Arg::parse( HINSTANCE hinstance , const std::string & command_line )
{
	m_array.clear() ;
	m_array.push_back( moduleName(hinstance) ) ;
	parseCore( command_line ) ;
	setPrefix() ;
}

/// \file garg_win32.cpp
