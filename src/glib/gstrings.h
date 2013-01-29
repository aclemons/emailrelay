//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gstrings.h
///

#ifndef G_STRINGS_H
#define G_STRINGS_H

#include "gdef.h"
#include <string>
#include <list>
#include <vector>
#include <map>
#include <stdexcept>

/// \namespace G
namespace G
{

/// 
/// A std::list of std::strings.
/// \see Str
///
typedef std::list<std::string> Strings ;

/// 
/// A std::vector of std::strings.
///
typedef std::vector<std::string> StringArray ;

/// 
/// A std::map of std::strings.
///
typedef std::map<std::string,std::string> StringMap ;

}

/// \namespace G
namespace G
{
	class StringMapReader ;
}

/// \class G::StringMapReader
/// An adaptor for reading a const StringMap with at().
///
class G::StringMapReader 
{
public:
	StringMapReader( const StringMap & map_ ) ;
		///< Implicit constructor.

	const std::string & at( const std::string & key ) const ;
		///< Returns the value, or throws.

	const std::string & at( const std::string & key , const std::string & default_ ) const ;
		///< Returns the value, or the default.

	Strings keys( unsigned int limit = 0U , const char * elipsis = NULL ) const ;
		///< Returns a list of keys (optionally up to some limit).

private: 
	const StringMap & m_map ;
} ;

#endif
