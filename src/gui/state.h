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
/// Provides access to the state file. Non-static
/// methods are read-only. Refer to comments in guimain.cpp.
///
class State 
{
public:
	typedef std::map<std::string,std::string> Map ;

	static G::Path file( const std::string & argv0 ) ;
		///< Returns the name of the state file associated with the given gui executable.

	static Map read( std::istream & ) ;
		///< Reads from a state file.

	static void write( std::ostream & , const std::string & contents , const G::Path & exe ) ;
		///< Writes a complete state file.

	static void write( std::ostream & , const Map & state_map , const G::Path & exe , const std::string & stop ) ;
		///< Writes a complete state file. Suppresses
		///< items where the key contains the given
		///< stop word.

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

