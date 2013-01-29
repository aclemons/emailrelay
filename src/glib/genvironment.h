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
/// \file genvironment.h
///

#ifndef G_ENVIRONMENT_H
#define G_ENVIRONMENT_H

#include "gdef.h"
#include <string>

/// \namespace G
namespace G
{
	class Environment ;
}

/// \class G::Environment
/// A static class to wrap getenv() and putenv().
///
class G::Environment 
{
public:

	static std::string get( const std::string & name , const std::string & default_ ) ;
		///< Returns the environment variable value or the given default.

	static void put( const std::string & name , const std::string & value ) ;
		///< Sets the environment variable value.

private:
	Environment() ; // not implemented
} ;

#endif
