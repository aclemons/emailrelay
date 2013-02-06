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
/// \file mapfile.h
///

#ifndef G_GUI_MAP_FILE_H
#define G_GUI_MAP_FILE_H

#include "gdef.h"
#include "gpath.h"
#include "gstrings.h"
#include <map>
#include <string>
#include <iostream>

/// \class MapFile
/// A static interface for handling key=value files.
///
class MapFile 
{
public:
	static G::StringMap read( std::istream & ) ;
		///< Reads the stream into a map.

	static void read( G::StringMap & , std::istream & , bool underscore_to_dash , bool to_lower , 
		const std::string & section_prefix = std::string() , bool in_section_predicate = true ) ;
			///< Reads the stream into a map. Reads only the required section if the
			///< section prefix is given.

	static void writeItem( std::ostream & , const std::string & key , const std::string & value ) ;
		///< Writes a single item to the stream.

	static void edit( const G::Path & path , const G::StringMap & map , 
		const std::string & section_prefix , bool in_section_predicate ,
		const G::StringMap & stop_list , bool make_backup , bool allow_read_error ,
		bool allow_write_error ) ;
			///< Edits a file, or a section of it, so that it ends up
			///< containing the map values, excluding any values that
			///< also appear in the stop-list.

private:
	MapFile() ;
	typedef std::list<std::string> List ;
	static std::string quote( const std::string & ) ;
	static G::StringMap purge( const G::StringMap & map_in , const G::StringMap & stop_list ) ;
	static List lines( const G::Path & , bool ) ;
	static void commentOut( List & , const std::string & section_prefix , bool in_section_predicate ) ;
	static void replace( List & , const G::StringMap & ) ;
	static void backup( const G::Path & ) ;
	static void save( const G::Path & , List & , bool ) ;
} ;

#endif
