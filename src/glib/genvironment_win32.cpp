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
//
// genvironment_win32.cpp
//

#include "gdef.h"
#include "genvironment.h"
#include <cstdlib>
#include <vector>

std::string G::Environment::get( const std::string & name , const std::string & default_ )
{
	size_t n = 0U ;
	errno_t rc = ::getenv_s( &n , NULL , 0U , name.c_str() ) ;
	if( n == 0U ) // rc will be ERANGE if the environment variable exists
		return default_ ;

	std::vector<char> buffer( n ) ;
	rc = ::getenv_s( &n , &buffer[0] , n , name.c_str() ) ;
	if( rc != 0 || n == 0U )
		return default_ ;

	buffer.push_back( '\0' ) ; // just in case
	return std::string( &buffer[0] ) ;
}

/// \file genvironment_win32.cpp
