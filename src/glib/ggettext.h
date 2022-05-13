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
/// \file ggettext.h
///

#ifndef G_GETTEXT_H
#define G_GETTEXT_H

#include "gdef.h"
#include <string>

namespace G
{
	void gettext_init( const std::string & localedir , const std::string & name ) ;
		///< Initialises the gettext() library. This uses environment variables
		///< to set the CTYPE and MESSAGES facets of the global C locale as a
		///< side-effect.

	const char * gettext( const char * ) noexcept ;
		///< Returns the message translation in the current locale's codeset,
		///< eg. ISO-8859-1 or UTF-8, transcoding from the catalogue as
		///< necessary. See also txt().

	constexpr const char * gettext_noop( const char * p ) ;
		///< Marks a string for potential translation at build-time (see xgettext),
		///< but no translation is applied at run-time. See also tx().

	const char * txt( const char * p ) ;
		///< Calls G::gettext(). Typically used unscoped by virtue of
		///< a 'using' declaration, for the benefit of xgettext.

	constexpr const char * tx( const char * p ) ;
		///< Returns the parameter. Used as a marker for xgettext.
}

constexpr const char * G::gettext_noop( const char * p )
{
	return p ;
}

inline const char * G::txt( const char * p )
{
	return G::gettext( p ) ;
}

constexpr const char * G::tx( const char * p )
{
	return p ;
}

#endif
