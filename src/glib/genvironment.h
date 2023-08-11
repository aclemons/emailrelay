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
/// \file genvironment.h
///

#ifndef G_ENVIRONMENT_H
#define G_ENVIRONMENT_H

#include "gdef.h"
#include "gexception.h"
#include <string>
#include <vector>
#include <map>

namespace G
{
	class Environment ;
}

//| \class G::Environment
/// Holds a set of environment variables and also provides static methods
/// to wrap getenv() and putenv().
///
class G::Environment
{
public:
	G_EXCEPTION( Error , tx("invalid environment variable") ) ;

	static std::string get( const std::string & name , const std::string & default_ ) ;
		///< Returns the environment variable value or the given default.

	static void put( const std::string & name , const std::string & value ) ;
		///< Sets the environment variable value.

	static Environment minimal( bool sbin = false ) ;
		///< Returns a minimal, safe set of environment variables.

	static Environment inherit() ;
		///< Returns an empty() environment, as if default constructed.
		///< This is syntactic sugar for the G::NewProcess interface.

	explicit Environment( const std::map<std::string,std::string> & ) ;
		///< Constructor from a map.

	~Environment() = default ;
		///< Destructor.

	void add( const std::string & name , const std::string & value ) ;
		///< Adds a variable to this set. Does nothing if already
		///< present.

	bool contains( const std::string & name ) const ;
		///< Returns true if the given variable is in this set.

	std::string value( const std::string & name , const std::string & default_ = {} ) const ;
		///< Returns the value of the given variable in this set.

	void set( const std::string & name , const std::string & value ) ;
		///< Inserts or updates a variable in this set.

	const char * ptr() const noexcept ;
		///< Returns a contiguous block of memory containing the
		///< null-terminated strings with an extra zero byte
		///< at the end.

	char ** v() const noexcept ;
		///< Returns a null-terminated array of pointers.

	bool empty() const noexcept ;
		///< Returns true if empty.

	Environment( const Environment & ) ;
		///< Copy constructor.

	Environment( Environment && ) noexcept ;
		///< Move constructor.

	Environment & operator=( const Environment & ) ;
		///< Assigment operator.

	Environment & operator=( Environment && ) noexcept ;
		///< Move assigment operator.

	bool valid() const ;
		///< Returns true if the class invariants are
		///< satisfied. Used in testing.

private:
	Environment() ;
	void swap( Environment & ) noexcept ;

private:
	using Map = std::map<std::string,std::string> ;
	using List = std::vector<std::string> ;
	static char * stringdup( const std::string & ) ;
	void setup() ;
	void setList() ;
	void setPointers() ;
	void setBlock() ;

private:
	Map m_map ;
	std::vector<std::string> m_list ;
	std::vector<char*> m_pointers ;
	std::string m_block ;
} ;

inline
bool G::Environment::empty() const noexcept
{
	return m_map.empty() ;
}

inline
G::Environment G::Environment::inherit()
{
	return {} ;
}

#endif
