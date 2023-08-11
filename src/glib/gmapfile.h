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
/// \file gmapfile.h
///

#ifndef G_MAP_FILE_H
#define G_MAP_FILE_H

#include "gdef.h"
#include "gpath.h"
#include "gstringarray.h"
#include "gstringmap.h"
#include "gexception.h"
#include "goptions.h"
#include "goptionvalue.h"
#include "goptionmap.h"
#include <string>
#include <iostream>

namespace G
{
	class MapFile ;
}

//| \class G::MapFile
/// A class for reading, writing and editing key=value files,
/// supporting variable expansion of percent-key-percent values,
/// comments, creation of backup files, and logging.
///
/// Also supports initialisation from a G::OptionMap, containing
/// G::OptionValue values. See also G::OptionParser.
///
/// Values containing whitespace are/can-be simply quoted with
/// initial and terminal double-quote characters, but with no
/// special handling of escapes or embedded quotes. For full
/// transparency values must not start with whitespace or '=',
/// must not end with whitespace, must not start-and-end with
/// double-quotes, must not contain commas, and should not
/// contain percent characters if using expand() methods.
///
class G::MapFile
{
public:
	struct Error : public Exception /// Exception class for G::MapFile.
	{
		using Exception::Exception ;
	} ;

	MapFile() ;
		///< Constructor for an empty map.

	explicit MapFile( const StringMap & map ) ;
		///< Constructor that initialises from a string map.

	explicit MapFile( const OptionMap & map , string_view yes = {} ) ;
		///< Constructor that initialises from an option value map,
		///< typically parsed out from a command-line. Unvalued 'on'
		///< options in the option value map are loaded into this
		///< mapfile object with a value given by the 'yes' parameter,
		///< whereas unvalued 'off' options are not loaded at all.
		///< Multi-valued options are loaded as a comma-separated
		///< list.

	explicit MapFile( const Path & , string_view kind = {} ) ;
		///< Constructor that reads from a file. Lines can have a key
		///< and no value (see booleanValue()). Comments must be at
		///< the start of the line. Values are left and right-trimmed,
		///< but can otherwise contain whitespace. The trailing parameter
		///< is used in error messages to describe the kind of file,
		///< defaulting to "map".

	explicit MapFile( std::istream & ) ;
		///< Constructor that reads from a stream.

	const StringArray & keys() const ;
		///< Returns a reference to the internal ordered list of keys.

	static void check( const Path & , string_view kind = {} ) ;
		///< Throws if the file is invalid. This is equivalent to
		///< constructing a temporary MapFile object, but it
		///< specifically does not do any logging.

	void add( string_view key , string_view value , bool clear = false ) ;
		///< Adds or updates a single item in the map.
		///< If updating then by default the new value
		///< is appended with a comma separator.

	void writeItem( std::ostream & , string_view key ) const ;
		///< Writes a single item from this map to the stream.

	static void writeItem( std::ostream & , string_view key , string_view value ) ;
		///< Writes an arbitrary item to the stream.

	void editInto( const Path & path , bool make_backup ,
		bool allow_read_error , bool allow_write_error ) const ;
			///< Edits an existing file so that its contents
			///< reflect this map.

	bool contains( string_view key ) const ;
		///< Returns true if the map contains the given key.

	Path pathValue( string_view key ) const ;
		///< Returns a mandatory path value from the map.
		///< Throws if it does not exist.

	Path pathValue( string_view key , const Path & default_ ) const ;
		///< Returns a path value from the map.

	unsigned int numericValue( string_view key , unsigned int default_ ) const ;
		///< Returns a numeric value from the map.

	std::string value( string_view key , string_view default_ = {} ) const ;
		///< Returns a string value from the map. Returns the default
		///< if there is no such key or if the value is empty.

	bool booleanValue( string_view key , bool default_ ) const ;
		///< Returns a boolean value from the map. Returns true if
		///< the key exists with an empty value. Returns the default
		///< if no such key.

	void remove( string_view key ) ;
		///< Removes a value (if it exists).

	const StringMap & map() const ;
		///< Returns a reference to the internal map.

	void log( const std::string & prefix = {} ) const ;
		///< Logs the contents.

	std::string expand( string_view value ) const ;
		///< Does one-pass variable substitution for the given string.
		///< Sub-strings like "%xyz%" are replaced by 'value("xyz")'
		///< and "%%" is replaced by "%". If there is no appropriate
		///< value in the map then the sub-string is left alone
		///< (so "%xyz%" remains as "%xyz%" if there is no "xyz"
		///< map item).

	Path expandedPathValue( string_view key ) const ;
		///< Returns a mandatory path value from the map
		///< with expand(). Throws if it does not exist.

	Path expandedPathValue( string_view key , const Path & default_ ) const ;
		///< Returns a path value from the map with expand().

private:
	using List = std::list<std::string> ;
	void readFrom( const Path & , string_view ) ;
	void readFrom( std::istream & ss ) ;
	static std::string quote( const std::string & ) ;
	List read( const Path & , string_view , bool ) const ;
	void commentOut( List & ) const ;
	void replace( List & ) const ;
	bool expand_( std::string & ) const ;
	std::string expandAll( string_view ) const ;
	static void backup( const Path & ) ;
	static void save( const Path & , List & , bool ) ;
	std::string mandatoryValue( string_view ) const ;
	bool ignore( const std::string & ) const ;
	static std::string ekind( string_view ) ;
	static std::string epath( const Path & ) ;
	static Error readError( const Path & , string_view ) ;
	static Error writeError( const Path & , string_view = {} ) ;
	static Error missingValueError( const Path & , const std::string & , const std::string & ) ;
	StringMap::const_iterator find( string_view ) const ;
	StringMap::iterator find( string_view ) ;

private:
	Path m_path ; // if any
	std::string m_kind ;
	StringMap m_map ;
	StringArray m_keys ; // kept in input order
} ;

#endif
