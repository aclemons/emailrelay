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

//| \class G::OptionMap
/// A multimap-like container for command-line options and their values.
/// The values are G::OptionValue objects, so they can be valued with a
/// string value or unvalued with an on/off status, and they can have
/// a repeat-count. Normally populated by G::OptionParser.
///
class G::OptionMap
{
public:
	using Map = std::multimap<std::string,OptionValue> ;
	using value_type = Map::value_type ;
	using iterator = Map::iterator ;
	using const_iterator = Map::const_iterator ;

public:
	void insert( const Map::value_type & ) ;
		///< Inserts the key/value pair into the map. The ordering of values in
		///< the map with the same key is in the order of insert()ion.

	void replace( const std::string & key , const std::string & value ) ;
		///< Replaces all matching values with a single value.

	void increment( const std::string & key ) ;
		///< Increments the repeat count for the given entry.

	const_iterator begin() const ;
		///< Returns the begin iterator.

	const_iterator cbegin() const ;
		///< Returns the begin iterator.

	const_iterator end() const ;
		///< Returns the off-the-end iterator.

	const_iterator cend() const ;
		///< Returns the off-the-end iterator.

	const_iterator find( const std::string & ) const ;
		///< Finds the map entry with the given key.

	void clear() ;
		///< Clears the map.

	bool contains( const std::string & ) const ;
		///< Returns true if the map contains the given key, but ignoring 'off'
		///< option-values.

	bool contains( const char * ) const ;
		///< Overload taking a c-string.

	std::size_t count( const std::string & key ) const ;
		///< Returns the total repeat count for all matching entries.
		///< See G::OptionValue::count().

	std::string value( const std::string & key , const std::string & default_ = {} ) const ;
		///< Returns the matching value, with concatentation into a comma-separated
		///< list if multivalued. If there are any on/off option-values matching
		///< the key then a single value is returned corresponding to the first
		///< one, being G::Str::positive() if 'on' or the supplied default
		///< if 'off'.

	std::string value( const char * key , const char * default_ = nullptr ) const ;
		///< Overload taking c-strings.

private:
	std::string join( Map::const_iterator , Map::const_iterator , const std::string & ) const ;
	static bool compare( const Map::value_type & , const Map::value_type & ) ;

private:
	Map m_map ;
} ;

#endif
