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
/// \file gmapfile.h
///

#ifndef G_MAP_FILE_H
#define G_MAP_FILE_H

#include "gdef.h"
#include "gpath.h"
#include "gstringarray.h"
#include "gstringmap.h"
#include "gstringview.h"
#include "gexception.h"
#include "goptions.h"
#include "goptionvalue.h"
#include "goptionmap.h"
#include <string>
#include <utility>
#include <iostream>
#include <vector>

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

	explicit MapFile( const OptionMap & map , std::string_view yes = {} ) ;
		///< Constructor that initialises from an option value map,
		///< typically parsed out from a command-line. Unvalued 'on'
		///< options in the option value map are loaded into this
		///< mapfile object with a value given by the 'yes' parameter,
		///< whereas unvalued 'off' options are not loaded at all.
		///< Multi-valued options are loaded as a comma-separated
		///< list.

	explicit MapFile( const Path & , std::string_view kind = {} ) ;
		///< Constructor that reads from a file. Lines can have a key
		///< and no value (see booleanValue()). Comments must be at
		///< the start of the line. Values are left and right-trimmed,
		///< but can otherwise contain whitespace. The trailing parameter
		///< is used in error messages to describe the kind of file,
		///< defaulting to "map".

	MapFile( const Path & , std::string_view kind , std::nothrow_t ) ;
		///< A non-throwing overload that reads from a file and
		///< ignores any errors.

	explicit MapFile( std::istream & ) ;
		///< Constructor that reads from a stream.

	const StringArray & keys() const ;
		///< Returns a reference to the internal ordered list of keys.

	void add( std::string_view key , std::string_view value , bool clear = false ) ;
		///< Adds or updates a single item in the map.
		///< If updating then by default the new value
		///< is appended with a comma separator.

	bool update( std::string_view key , std::string_view value ) ;
		///< Updates an existing value. Returns false if not
		///< found.

	bool remove( std::string_view key ) ;
		///< Removes a value (if it exists).

	void writeItem( std::ostream & , std::string_view key ) const ;
		///< Writes a single item from this map to the stream.

	static void writeItem( std::ostream & , std::string_view key , std::string_view value ) ;
		///< Writes an arbitrary item to the stream.

	Path editInto( const Path & path , bool make_backup , bool do_throw = true ) const ;
		///< Edits an existing file so that its contents reflect
		///< this map. Returns the path of the backup file, if
		///< created.

	bool contains( std::string_view key ) const ;
		///< Returns true if the map contains the given key.

	Path pathValue( std::string_view key ) const ;
		///< Returns a mandatory path value from the map.
		///< Throws if it does not exist.

	Path pathValue( std::string_view key , const Path & default_ ) const ;
		///< Returns a path value from the map.

	unsigned int numericValue( std::string_view key , unsigned int default_ ) const ;
		///< Returns a numeric value from the map.

	std::string value( std::string_view key , std::string_view default_ = {} ) const ;
		///< Returns a string value from the map. Returns the default
		///< if there is no such key or if the value is empty.

	bool valueContains( std::string_view key , std::string_view token , std::string_view default_ = {} ) const ;
		///< Returns true if value(key,default_) contains the given
		///< comma-separated token.

	bool booleanValue( std::string_view key , bool default_ ) const ;
		///< Returns a boolean value from the map. Returns true if
		///< the key exists with an empty value. Returns the default
		///< if no such key.

	const StringMap & map() const ;
		///< Returns a reference to the internal map.

	void log( const std::string & prefix = {} ) const ;
		///< Logs the contents.

	std::string expand( std::string_view value ) const ;
		///< Does one-pass variable substitution for the given string.
		///< Sub-strings like "%xyz%" are replaced by 'value("xyz")'
		///< and "%%" is replaced by "%". If there is no appropriate
		///< value in the map then the sub-string is left alone
		///< (so "%xyz%" remains as "%xyz%" if there is no "xyz"
		///< map item).

	Path expandedPathValue( std::string_view key ) const ;
		///< Returns a mandatory path value from the map
		///< with expand(). Throws if it does not exist.

	Path expandedPathValue( std::string_view key , const Path & default_ ) const ;
		///< Returns a path value from the map with expand().

private:
	using List = std::vector<std::string> ;
	void readFromFile( const Path & , std::string_view , bool do_throw = true ) ;
	void readFromStream( std::istream & ss ) ;
	List readLines( const Path & , std::string_view , bool do_throw ) const ;
	static std::pair<std::string_view,std::string_view> split( std::string_view ) ;
	static std::string join( std::string_view , std::string_view ) ;
	static std::string quote( const std::string & ) ;
	bool expand_( std::string & ) const ;
	std::string mandatoryValue( std::string_view ) const ;
	static bool valued( const std::string & ) ;
	static bool commentedOut( const std::string & ) ;
	static std::string strkind( std::string_view ) ;
	static std::string strpath( const Path & ) ;
	static Error readError( const Path & , std::string_view ) ;
	static Error writeError( const Path & , std::string_view = {} ) ;
	static Error missingValueError( const Path & , const std::string & , const std::string & ) ;
	StringMap::const_iterator find( std::string_view ) const ;
	StringMap::iterator find( std::string_view ) ;
	static Path toPath( std::string_view ) ;

private:
	Path m_path ; // if any
	std::string m_kind ;
	StringMap m_map ;
	StringArray m_keys ; // kept in input order
} ;

#endif
