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
/// \file ggettext_unix.cpp
///

#include "gdef.h"
#include "ggettext.h"
#include <clocale>
#include <libintl.h>

void G::gettext_init( const std::string & localedir , const std::string & appname )
{
	if( !appname.empty() )
	{
		std::setlocale( LC_MESSAGES , "" ) ;
		std::setlocale( LC_CTYPE , "" ) ;
		if( !localedir.empty() )
            bindtextdomain( appname.c_str() , localedir.c_str() ) ;
		textdomain( appname.c_str() ) ;
	}
}

const char * G::gettext( const char * p ) noexcept
{
		return ::gettext( p ) ;
}

