//
// Copyright (C) 2001-2008 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file state.h
///

#ifndef G_GUI_STATE_H
#define G_GUI_STATE_H

#include "gpath.h"
#include <map>
#include <string>

/// \class State
/// Provides read-only access to the state file.
///
class State 
{
public:
	typedef std::map<std::string,std::string> Map ;

	static G::Path file( const std::string & argv0 ) ;
		///< Returns the name of a state file.

	static Map read( std::istream & ) ;
		///< Reads from a state file.

	static void write( std::ostream & , const std::string & key , const std::string & value , 
		const std::string & prefix , const std::string & eol ) ;
			///< Writes a value to the state file.

	explicit State( const Map & ) ;
		///< Constructor.

	G::Path value( const std::string & key , const G::Path & default_ ) const ;
		///< Returns a path value from the map.

	std::string value( const std::string & key , const std::string & default_ = std::string() ) const ;
		///< Returns a string value from the map.

	std::string value( const std::string & key , const char * default_ ) const ;
		///< Returns a string value from the map.

	bool value( const std::string & key , bool default_ ) const ;
		///< Returns a boolean value from the map.

private:
	State( const State & ) ;
	void operator=( const State & ) ;

private:
	const Map & m_map ;
} ;

#endif

