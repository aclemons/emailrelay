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
/// \file genvironment_unix.cpp
///

#include "gdef.h"
#include "genvironment.h"
#include <cstdlib> // std::getenv()
#include <cstring>
#include <stdexcept>

std::string G::Environment::get( const std::string & name , const std::string & default_ )
{
	const char * p = std::getenv( name.c_str() ) ;
	return p ? std::string(p) : default_ ;
}

char * G::Environment::stringdup( const std::string & s )
{
	void * p = std::memcpy( new char[s.size()+1U] , s.c_str() , s.size()+1U ) ; // NOLINT
	return static_cast<char*>(p) ;
}

void G::Environment::put( const std::string & name , const std::string & value )
{
	// see man putenv(3) NOTES
	char * deliberately_leaky_copy = stringdup( std::string().append(name).append(1U,'=').append(value) ) ; // NOLINT
	::putenv( deliberately_leaky_copy ) ;
} // NOLINT

G::Environment G::Environment::minimal( bool sbin )
{
	Environment env ;
	env.add( "PATH" , sbin ? "/usr/bin:/bin:/usr/sbin:/sbin" : "/usr/bin:/bin" ) ; // no "."
	env.add( "IFS" , " \t\n" ) ;
	return env ;
}

