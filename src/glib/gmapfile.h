//
// Copyright (C) 2001-2015 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_MAP_FILE_H__
#define G_MAP_FILE_H__

#include "gdef.h"
#include "gpath.h"
#include "gstrings.h"
#include "goptionvalue.h"
#include <map>
#include <string>
#include <iostream>

/// \namespace G
namespace G
{
	class MapFile ;
}

/// \class G::MapFile
/// A class for reading and editing key=value files, supporting
/// the creation of backup files, variable expansion, character-set encoding 
/// and logging.
///
class G::MapFile
{
public:
	MapFile() ;
		///< Constructor for an empty map.

	explicit MapFile( const G::StringMap & map ) ;
		///< Constructor that reads from a string map.

	explicit MapFile( const std::map<std::string,G::OptionValue> & map , const std::string & yes = std::string() ) ;
		///< Constructor that reads from an option-value map.
		///< Unvalued 'true' options are loaded into the map with
		///< a value given by the second parameter, whereas
		///< unvalued 'false' options are not loaded.

	explicit MapFile( const G::Path & , bool utf8 = false ) ;
		///< Constructor that reads from a file.

	explicit MapFile( std::istream & , bool utf8 = false ) ;
		///< Constructor that reads from a stream.

	const G::StringArray & keys() const ;
		///< Returns a reference to the ordered list of keys.

	static void check( const G::Path & , bool utf8 = false ) ;
		///< Throws if the file is invalid. This is equivalent
		///< to constructing a temporary MapFile object, but it
		///< specifically does not do any logging.

	void add( const std::string & key , const std::string & value ) ;
		///< Adds or updates a single item in the map.

	void writeItem( std::ostream & , const std::string & key , bool utf8 = false ) const ;
		///< Writes a single item from this map to the stream.

	static void writeItem( std::ostream & , const std::string & key , const std::string & value , bool utf8 = false ) ;
		///< Writes an arbitrary item to the stream.

	void editInto( const G::Path & path , bool make_backup = true ,
		bool allow_read_error = false , bool allow_write_error = false , bool utf8 = false ) const ;
			///< Edits an existing file so that its contents reflect this map.

	bool contains( const std::string & key ) const ;
		///< Returns true if the map contains the given key.

	G::Path pathValue( const std::string & key ) const ;
		///< Returns a mandatory path value from the map. Throws if it does not exist.

	G::Path pathValue( const std::string & key , const G::Path & default_ ) const ;
		///< Returns a path value from the map.

	unsigned int numericValue( const std::string & key , unsigned int default_ ) const ;
		///< Returns a numeric value from the map.

	std::string value( const std::string & key , const std::string & default_ = std::string() ) const ;
		///< Returns a string value from the map.

	std::string value( const std::string & key , const char * default_ ) const ;
		///< Returns a string value from the map.

	bool booleanValue( const std::string & key , bool default_ ) const ;
		///< Returns a boolean value from the map.

	void remove( const std::string & key ) ;
		///< Removes a value (if it exists).

	const G::StringMap & map() const ;
		///< Returns a reference to the internal map.

	void log() const ;
		///< Logs the contents.

	std::string expand( const std::string & value ) const ;
		///< Does one-pass variable substitution for the given string.
		///< Sub-strings like "%xyz%" are replaced by value("xyz").
		///< If there is no such value in the map then the sub-string 
		///< is left alone.

private:
	bool m_logging ;
	G::StringMap m_map ;
	G::StringArray m_keys ;
	typedef std::list<std::string> List ;
	void readFrom( const G::Path & , bool ) ;
	void readFrom( std::istream & ss , bool ) ;
	static std::string quote( const std::string & ) ;
	List read( const G::Path & , bool ) const ;
	void commentOut( List & ) const ;
	void replace( List & , bool ) const ;
	bool expand_( std::string & ) const ;
	std::string expandAll( const std::string & ) const ;
	static void backup( const G::Path & ) ;
	static void save( const G::Path & , List & , bool ) ;
	void log( const std::string & , const std::string & ) const ;
	static void log( bool , const std::string & , const std::string & ) ;
	static std::string fromUtf( const std::string & value , bool utf8 ) ;
	static std::string toUtf( const std::string & value , bool utf8 ) ;
} ;

#endif
