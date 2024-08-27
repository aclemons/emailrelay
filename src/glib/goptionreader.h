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
/// \file goptionreader.h
///

#ifndef G_OPTION_READER_H
#define G_OPTION_READER_H

#include "gdef.h"
#include "gstringarray.h"
#include "gexception.h"
#include "ggettext.h"
#include "gpath.h"

namespace G
{
	class OptionReader ;
}

//| \class G::OptionReader
/// Provides a static function to read options from a config file.
///
class G::OptionReader
{
public:
	G_EXCEPTION( FileError , tx("error reading configuration file") )

	static StringArray read( const G::Path & , std::size_t limit = 1000U ) ;
		///< Reads options from file as a list of strings like "--foo=bar".
		///< Throws on error.

	static std::size_t add( StringArray & out , const G::Path & , std::size_t limit = 1000U ) ;
		///< Adds options read from file to an existing list.
		///< Returns the number of options added.

public:
	OptionReader() = delete ;
} ;

#endif
