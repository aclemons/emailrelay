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
/// \file gstringwrap.h
///

#ifndef G_STRING_WRAP_H
#define G_STRING_WRAP_H

#include "gdef.h"
#include "gstringview.h"
#include <string>

namespace G
{
	class StringWrap ;
}

//| \class G::StringWrap
/// A word-wrap class.
///
class G::StringWrap
{
public:
	static std::string wrap( const std::string & text ,
		const std::string & prefix_first , const std::string & prefix_other ,
		std::size_t width_first = 70U , std::size_t width_other = 0U ,
		bool preserve_spaces = false , const std::locale & = defaultLocale() ) ;
			///< Does word-wrapping. The return value is a string with embedded
			///< newlines. If 'preserve_spaces' is true then all space characters
			///< between input words that end up in the middle of an output line
			///< are preserved. There is no special handling of tabs or carriage
			///< returns. The 'first/other' parameters distinguish between the
			///< first output line and the rest.

	static std::locale defaultLocale() ;
		///< Returns a locale with at least the CTYPE and codecvt facets
		///< initialised according to the C locale's CTYPE. Returns the
		///< classic locale on error.
		///<
		///< A motivating use-case is where the gettext() library has been
		///< initialised by setting the C locale's CTYPE and MESSAGES facets,
		///< meaning that all gettext() strings are automatically converted
		///< to the relevant character encoding. If the strings from
		///< gettext() are to be word-wrapped accurately then the
		///< word-wrapper's CTYPE and codecvt facets need to match the
		///< C locale's CTYPE.

	static std::size_t wordsize( const std::string & mbcs , const std::locale & ) ;
		///< Returns the number of wide characters after converting the input
		///< string using the locale's codecvt facet. Conversion errors are
		///< ignored.

public:
	StringWrap() = delete ;

private:
	struct Config ;
	class WordWrapper ;
	static std::size_t wordsize( G::string_view mbcs , const std::locale & ) ;
	static void wrap( std::istream & , WordWrapper & ) ;
} ;

#endif
