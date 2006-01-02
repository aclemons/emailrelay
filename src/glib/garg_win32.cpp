//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// garg_win32.cpp
//

#include "gdef.h"
#include "garg.h"
#include "gdebug.h"

std::string G::Arg::moduleName( HINSTANCE hinstance )
{
	char buffer[260] ;
	size_t size = sizeof(buffer) ;
	*buffer = '\0' ;
	::GetModuleFileName( hinstance , buffer , size-1U ) ;
	buffer[size-1U] = '\0' ;
	return std::string(buffer) ;
}

