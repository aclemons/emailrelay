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
		bool preserve_spaces = false ) ;
			///< Does word-wrapping of UTF-8 text. The return value is a string
			///< with embedded newlines. If 'preserve_spaces' is true then all
			///< space characters between input words that end up in the middle
			///< of an output line are preserved. There is no special handling
			///< of tabs or carriage returns. The 'first/other' parameters
			///< distinguish between the first output line and the rest.

	static std::size_t wordsize( const std::string & ) ;
		///< Returns the number of characters in UTF-8 text.

public:
	StringWrap() = delete ;
} ;

#endif
