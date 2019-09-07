//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file goptionmap.h
///

#ifndef G_OPTION_MAP_H
#define G_OPTION_MAP_H

#include "gdef.h"
#include "goptionvalue.h"
#include <string>
#include <map>
#include <algorithm>

namespace G
{
	class OptionMap ;
}

/// \class G::OptionMap
/// A multimap-like container for command-line options and their values.
/// The values are G::OptionValue objects, so they can be valued with a
/// string value or unvalued with an on/off status.
/// Normally populated by G::OptionParser.
///
class G::OptionMap
{
public:
	typedef std::multimap<std::string,OptionValue> Map ;
	typedef Map::value_type value_type ;
	typedef Map::iterator iterator ;
	typedef Map::const_iterator const_iterator ;

public:
	OptionMap() ;
		///< Default constructor for an empty map.

	void insert( const Map::value_type & ) ;
		///< Inserts the key/value pair into the map. The ordering of values in the
		///< map with the same key is normally the order of insertion (but this
		///< can depend on the underlying multimap implementation).

	void replace( const std::string & key , const std::string & value ) ;
		///< Replaces all matching values with a single valued() value.

	const_iterator begin() const ;
		///< Returns the begin iterator.

	const_iterator end() const ;
		///< Returns the off-the-end iterator.

	const_iterator find( const std::string & ) const ;
		///< Finds the map entry with the given key.

	void clear() ;
		///< Clears the map.

	bool contains( const std::string & ) const ;
		///< Returns true if the map contains the given key, but ignoring un-valued()
		///< 'off' options.

	bool contains( const char * ) const ;
		///< Overload taking a c-string.

	size_t count( const std::string & key ) const ;
		///< Returns the number of times the key appears in the multimap.

	std::string value( const std::string & key , const std::string & default_ = std::string() ) const ;
		///< Returns the value of the valued() option identified by the given key.
		///< Multiple matching values are concatenated with a comma separator
		///< (normally in the order of insertion).

	std::string value( const char * key , const char * default_ = nullptr ) const ;
		///< Overload taking c-strings.

private:
	std::string join( Map::const_iterator , Map::const_iterator ) const ;
	static bool value_comp( const Map::value_type & , const Map::value_type & ) ;

private:
	Map m_map ;
} ;

#endif
