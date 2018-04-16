//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gxtext.h
///

#ifndef G_XTEXT_H
#define G_XTEXT_H

#include "gdef.h"
#include <string>

namespace G
{
	class Xtext ;
}

/// \class G::Xtext
/// An xtext codec class, encoding space as "+20" etc.
/// \see RFC-1891 section 5
///
class G::Xtext
{
public:
	static std::string encode( const std::string & ) ;
		///< Encodes the given string.

	static std::string decode( const std::string & ) ;
		///< Decodes the given string. Allows lowercase
		///< hex characters (eg. "+1a")

	static bool valid( const std::string & ) ;
		///< Returns true if a valid encoding. Allows
		///< lowercase hex characters (eg. "+1a")

private:
	Xtext() ;
} ;

#endif
