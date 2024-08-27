//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gstringview.h"
#include <string>

// String literals should be marked for translation using gettext() or
// gettext_noop(), but not using the "G::" namespace scoping so that
// 'xgettext(1)' will still work. For brevity G::txt() or G::tx()
// can be used instead. See also G::format.
//
// Eg:
/// \code
/// #include "ggettext.h"
/// using G::tx ;
/// using G::txt ;
/// Message msg( tx("world") ) ; // Message ctor calls gettext()
/// std::cout << txt("hello") << msg.translated() << "\n" ;
/// \endcode

namespace G
{
	void gettext_init( const std::string & localedir , const std::string & name ) ;
		///< Initialises the gettext() library. This uses environment variables
		///< to set the CTYPE and MESSAGES facets of the global C locale as a
		///< side-effect.

	const char * gettext( const char * ) noexcept ;
		///< Returns the message translation in the current locale's codeset,
		///< eg. ISO-8859-1 or UTF-8, transcoding from the catalogue as
		///< necessary.

	constexpr const char * gettext_noop( const char * p ) noexcept ;
		///< Returns the parameter. Used to mark a string-literal for
		///< translation, with the conversion at run-time done with
		///< a call to gettext() elsewhere in the code.
		///<
		///< \code
		///< using G::gettext_noop ;
		///< std::cout << call_gettext( gettext_noop("hello, world") ) ;
		///< \endcode

	const char * txt( const char * p ) noexcept ;
		///< A briefer alternative to G::gettext().

	constexpr const char * tx( const char * p ) noexcept ;
		///< A briefer alternative to G::gettext_noop().

	constexpr std::string_view tx( std::string_view sv ) noexcept ;
		///< String view overload.
}

inline const char * G::txt( const char * p ) noexcept
{
	return G::gettext( p ) ;
}

constexpr const char * G::gettext_noop( const char * p ) noexcept
{
	return p ;
}

constexpr const char * G::tx( const char * p ) noexcept
{
	return p ;
}

constexpr std::string_view G::tx( std::string_view sv ) noexcept
{
	return sv ;
}

#endif
