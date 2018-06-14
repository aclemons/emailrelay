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
/// \file gmapfile.h
///

#ifndef G_MAP_FILE_H
#define G_MAP_FILE_H

#include "gdef.h"
#include "gpath.h"
#include "gstrings.h"
#include "gexception.h"
#include "goptions.h"
#include "goptionvalue.h"
#include "goptionmap.h"
#include <map>
#include <string>
#include <iostream>

namespace G
{
	class MapFile ;
}

/// \class G::MapFile
/// A class for reading and editing key=value files, supporting comments,
/// creation of backup files, variable expansion and logging.
/// \see G::OptionValue
///
class G::MapFile
{
public:
	G_EXCEPTION( ReadError , "cannot open map file" ) ;
	G_EXCEPTION( WriteError , "cannot write map file" ) ;
	G_EXCEPTION( Missing , "cannot find map file item" ) ;

	MapFile() ;
		///< Constructor for an empty map.

	explicit MapFile( const G::StringMap & map ) ;
		///< Constructor that initialises from a string map.

	explicit MapFile( const OptionMap & map , const std::string & yes = std::string() ) ;
		///< Constructor that initialises from an option value map,
		///< typically parsed out from a command-line. Unvalued 'true'
		///< options in the option value map are loaded into this
		///< mapfile object with a value given by the 'yes' parameter,
		///< whereas unvalued 'false' options are not loaded.
		///< Multi-valued options are loaded as a comma-separated
		///< list.

	explicit MapFile( const G::Path & ) ;
		///< Constructor that reads from a file. Lines can have a key
		///< and no value (see booleanValue()). Comments must be at
		///< the start of the line. Values are trimmed, but can
		///< otherwise contain whitespace.

	explicit MapFile( std::istream & ) ;
		///< Constructor that reads from a stream.

	const G::StringArray & keys() const ;
		///< Returns a reference to the ordered list of keys.

	static void check( const G::Path & ) ;
		///< Throws if the file is invalid. This is equivalent to
		///< constructing a temporary MapFile object, but it
		///< specifically does not do any logging.

	void add( const std::string & key , const std::string & value ) ;
		///< Adds or updates a single item in the map.

	void writeItem( std::ostream & , const std::string & key ) const ;
		///< Writes a single item from this map to the stream.

	static void writeItem( std::ostream & , const std::string & key , const std::string & value ) ;
		///< Writes an arbitrary item to the stream.

	void editInto( const G::Path & path , bool make_backup ,
		bool allow_read_error , bool allow_write_error ) const ;
			///< Edits an existing file so that its contents
			///< reflect this map.

	bool contains( const std::string & key ) const ;
		///< Returns true if the map contains the given key.

	G::Path pathValue( const std::string & key ) const ;
		///< Returns a mandatory path value from the map.
		///< Throws if it does not exist.

	G::Path pathValue( const std::string & key , const G::Path & default_ ) const ;
		///< Returns a path value from the map.

	unsigned int numericValue( const std::string & key , unsigned int default_ ) const ;
		///< Returns a numeric value from the map.

	std::string value( const std::string & key , const std::string & default_ = std::string() ) const ;
		///< Returns a string value from the map. Returns the default
		///< if there is no such key or if the value is empty.

	std::string value( const std::string & key , const char * default_ ) const ;
		///< Returns a string value from the map.

	bool booleanValue( const std::string & key , bool default_ ) const ;
		///< Returns a boolean value from the map. Returns true if
		///< the key exists with an empty value. Returns the default
		///< if no such key.

	void remove( const std::string & key ) ;
		///< Removes a value (if it exists).

	const G::StringMap & map() const ;
		///< Returns a reference to the internal map.

	void log() const ;
		///< Logs the contents.

	std::string expand( const std::string & value ) const ;
		///< Does one-pass variable substitution for the given string.
		///< Sub-strings like "%xyz%" are replaced by 'value("xyz")'
		///< and "%%" is replaced by "%". If there is no appropriate
		///< value in the map then the sub-string is left alone
		///< (so "%xyz%" remains as "%xyz%" if there is no "xyz"
		///< map item).

	G::Path expandedPathValue( const std::string & key ) const ;
		///< Returns a mandatory path value from the map
		///< with expand(). Throws if it does not exist.

	G::Path expandedPathValue( const std::string & key , const G::Path & default_ ) const ;
		///< Returns a path value from the map with expand().

private:
	typedef std::list<std::string> List ;
	void readFrom( const G::Path & ) ;
	void readFrom( std::istream & ss ) ;
	static std::string quote( const std::string & ) ;
	List read( const G::Path & , bool ) const ;
	void commentOut( List & ) const ;
	void replace( List & ) const ;
	bool expand_( std::string & ) const ;
	std::string expandAll( const std::string & ) const ;
	static void backup( const G::Path & ) ;
	static void save( const G::Path & , List & , bool ) ;
	void log( const std::string & , const std::string & ) const ;
	static void log( bool , const std::string & , const std::string & ) ;
	std::string mandatoryValue( const std::string & ) const ;
	bool ignore( const std::string & ) const ;

private:
	bool m_logging ;
	G::StringMap m_map ;
	G::StringArray m_keys ; // kept in input order
} ;

#endif
