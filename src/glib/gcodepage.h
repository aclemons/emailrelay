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
/// \file gcodepage.h
///

#ifndef G_CODEPAGE_H
#define G_CODEPAGE_H

#include "gdef.h"
#include "gstringview.h"

namespace G
{
	namespace CodePage /// Windows codepage conversion functions.
	{
		std::string fromCodePage850( std::string_view s ) ;
			///< Converts from codepage 850 to UTF-8.

		std::string toCodePage850( std::string_view s ) ;
			///< Converts from UTF-8 to codepage 850.

		std::string fromCodePage1252( std::string_view s ) ;
			///< Converts from codepage 1252 to UTF-8.

		std::string toCodePage1252( std::string_view s ) ;
			///< Converts from UTF-8 to codepage 1252.

		std::string toCodePageOem( std::string_view ) ;
			///< Converts from UTF-8 to the active OEM codepage
			///< (see GetOEMCP(), 850 on unix).

		std::string fromCodePageOem( std::string_view ) ;
			///< Converts from the active OEM codepage
			///< (see GetOEMCP(), 850 on unix) to UTF-8.

		std::string toCodePageAnsi( std::string_view ) ;
			///< Converts from UTF-8 to the active "ansi" codepage
			///< (see GetACP(), 1252 on unix).

		std::string fromCodePageAnsi( std::string_view ) ;
			///< Converts from the active OEM codepage
			///< (see GetACP(), 1252 on unix) to UTF-8.

		constexpr char oem_error = '\xDB' ; // OEM error character -- full block
		constexpr char ansi_error = '\xBF' ; // Ansi error character -- inverted question mark
	}
}

#endif
