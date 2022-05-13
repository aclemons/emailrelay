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

	explicit MapFile( const G::StringMap & map ) ;
		///< Constructor that initialises from a string map.

	explicit MapFile( const OptionMap & map , const std::string & yes = {} ) ;
		///< Constructor that initialises from an option value map,
		///< typically parsed out from a command-line. Unvalued 'on'
		///< options in the option value map are loaded into this
		///< mapfile object with a value given by the 'yes' parameter,
		///< whereas unvalued 'off' options are not loaded at all.
		///< Multi-valued options are loaded as a comma-separated
		///< list.

	explicit MapFile( const G::Path & , const std::string & kind = {} ) ;
		///< Constructor that reads from a file. Lines can have a key
		///< and no value (see booleanValue()). Comments must be at
		///< the start of the line. Values are left and right-trimmed,
		///< but can otherwise contain whitespace. The trailing parameter
		///< is used in error messages to describe the kind of file,
		///< defaulting to "map".

	explicit MapFile( std::istream & ) ;
		///< Constructor that reads from a stream.

	const G::StringArray & keys() const ;
		///< Returns a reference to the internal ordered list of keys.

	static void check( const G::Path & , const std::string & kind = {} ) ;
		///< Throws if the file is invalid. This is equivalent to
		///< constructing a temporary MapFile object, but it
		///< specifically does not do any logging.

	void add( const std::string & key , const std::string & value , bool clear = false ) ;
		///< Adds or updates a single item in the map.
		///< If updating then by default the new value
		///< is appended with a comma separator.

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

	std::string value( const std::string & key , const std::string & default_ = {} ) const ;
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

	void log( const std::string & prefix = {} ) const ;
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
	using List = std::list<std::string> ;
	void readFrom( const G::Path & , const std::string & ) ;
	void readFrom( std::istream & ss ) ;
	static std::string quote( const std::string & ) ;
	List read( const G::Path & , const std::string & , bool ) const ;
	void commentOut( List & ) const ;
	void replace( List & ) const ;
	bool expand_( std::string & ) const ;
	std::string expandAll( const std::string & ) const ;
	static void backup( const G::Path & ) ;
	static void save( const G::Path & , List & , bool ) ;
	std::string mandatoryValue( const std::string & ) const ;
	bool ignore( const std::string & ) const ;
	static std::string ekind( const std::string & ) ;
	static std::string epath( const G::Path & ) ;
	static Error readError( const G::Path & , const std::string & ) ;
	static Error writeError( const G::Path & , const std::string & = {} ) ;
	static Error missingValueError( const G::Path & , const std::string & , const std::string & ) ;

private:
	G::Path m_path ; // if any
	std::string m_kind ;
	G::StringMap m_map ;
	G::StringArray m_keys ; // kept in input order
} ;

#endif
