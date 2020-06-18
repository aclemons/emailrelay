//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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
//
// genvironment_unix.cpp
//

#include "gdef.h"
#include "genvironment.h"
#include <cstdlib> // std::getenv()

#if defined(G_LOG) || defined(G_ASSERT)
#error cannot use logging here
#endif

std::string G::Environment::get( const std::string & name , const std::string & default_ )
{
	const char * p = std::getenv( name.c_str() ) ;
	return p ? std::string(p) : default_ ;
}

void G::Environment::put( const std::string & name , const std::string & value )
{
	std::string s = name + "=" + value ;
	char * deliberately_leaky_copy = ::strdup( s.c_str() ) ; // see man putenv(3) NOTES // NOLINT
	::putenv( deliberately_leaky_copy ) ;
} // NOLINT

G::Environment G::Environment::minimal()
{
	Environment env ;
	env.add( "PATH" , "/usr/bin:/bin" ) ; // no "."
	env.add( "IFS" , " \t\n" ) ;
	return env ;
}

/// \file genvironment_unix.cpp
