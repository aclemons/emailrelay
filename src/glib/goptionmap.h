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
/// \file goptionmap.h
///

#ifndef G_OPTION_MAP_H
#define G_OPTION_MAP_H

#include "gdef.h"
#include "goptionvalue.h"
#include "gstringview.h"
#include <string>
#include <map>
#include <algorithm>
#include <functional> // std::less

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
	using Map = std::multimap<std::string,OptionValue,std::less<std::string>> ; // NOLINT modernize-use-transparent-functors
	using value_type = Map::value_type ;
	using iterator = Map::iterator ;
	using const_iterator = Map::const_iterator ;

public:
	void insert( const Map::value_type & ) ;
		///< Inserts the key/value pair into the map. The ordering of values
		///< in the map with the same key is in the order of insert()ion.

	void replace( std::string_view key , const std::string & value ) ;
		///< Replaces all matching values with a single value.

	void increment( std::string_view key ) noexcept ;
		///< Increments the repeat count for the given entry.

	const_iterator begin() const noexcept ;
		///< Returns the begin iterator.

	const_iterator cbegin() const noexcept ;
		///< Returns the begin iterator.

	const_iterator end() const noexcept ;
		///< Returns the off-the-end iterator.

	const_iterator cend() const noexcept ;
		///< Returns the off-the-end iterator.

	const_iterator find( std::string_view ) const noexcept ;
		///< Finds the map entry with the given key.

	void clear() ;
		///< Clears the map.

	bool contains( std::string_view ) const noexcept ;
		///< Returns true if the map contains the given key, but ignoring 'off'
		///< option-values.

	bool contains( const char * ) const noexcept ;
		///< Overload for c-string.

	bool contains( const std::string & ) const noexcept ;
		///< Overload for std-string.

	std::size_t count( std::string_view key ) const noexcept ;
		///< Returns the total repeat count for all matching entries.
		///< See G::OptionValue::count().

	std::string value( std::string_view key , std::string_view default_ = {} ) const ;
		///< Returns the matching value, with concatentation into a comma-separated
		///< list if multivalued (with no escaping). If there are any on/off
		///< option-values matching the key then a single value is returned
		///< corresponding to the first one, being G::Str::positive() if 'on'
		///< or the supplied default if 'off'.

	unsigned int number( std::string_view key , unsigned int default_ ) const noexcept ;
		///< Returns the matching value as a number.

private:
	using Range = std::pair<Map::const_iterator,Map::const_iterator> ;
	Range findRange( std::string_view key ) const noexcept ;
	Map::iterator findFirst( std::string_view key ) noexcept ;
	static std::string join( Map::const_iterator , Map::const_iterator , std::string_view ) ;

private:
	Map m_map ;
} ;

#endif
