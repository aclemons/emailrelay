//
// Copyright (C) 2001-2021 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file ggettext.h
///

#ifndef G_GETTEXT_H
#define G_GETTEXT_H

#include "gdef.h"
#include <string>
#include <clocale>

#if GCONFIG_HAVE_GETTEXT
#include <libintl.h>
#endif

namespace G
{
	void gettext_init( const std::string & localedir , const std::string & name ) ;
		///< Initialises the gettext() library. This uses environment variables
		///< to set the CTYPE and MESSAGES facets of the global C locale as a
		///< side-effect.

	constexpr const char * gettext_noop( const char * p ) ;
		///< Marks a string for translation at build-time, but no translation is
		///< applied at run-time.

	const char * gettext( const char * ) ;
		///< Returns the message translation in the current locale's codeset,
		///< eg. ISO8859-1 or UTF-8, transcoding from the catalogue as
		///< necessary.
}

#if GCONFIG_HAVE_GETTEXT
inline void G::gettext_init( const std::string & localedir , const std::string & appname )
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
inline const char * G::gettext( const char * p )
{
		return ::gettext( p ) ;
}
#else
inline void G::gettext_init( const std::string & , const std::string & )
{
}
inline const char * G::gettext( const char * p )
{
		return p ;
}
#endif

constexpr const char * G::gettext_noop( const char * p )
{
	return p ;
}

#endif
