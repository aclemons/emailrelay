//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gdebug.h"

std::string G::Arg::moduleName( HINSTANCE hinstance )
{
	char buffer[10000U] ;
	size_t size = sizeof(buffer) ;
	*buffer = '\0' ;
	::GetModuleFileName( hinstance , buffer , size-1U ) ;
	buffer[size-1U] = '\0' ;
	return std::string(buffer) ;
}

/// \file garg_win32.cpp
