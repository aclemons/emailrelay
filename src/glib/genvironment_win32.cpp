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
/// \file genvironment_win32.cpp
///

#include "gdef.h"
#include "genvironment.h"
#include <cstdlib> // _environ
#include <cstring>
#include <vector>
#include <cerrno>

#if ! GCONFIG_HAVE_GETENV_S
inline errno_t getenv_s( std::size_t * n_out , char * buffer , std::size_t n_in , const char * name )
{
	if( n_out == nullptr || name == nullptr || (!buffer&&n_in) )
		return EINVAL ;

	const char * p = ::getenv( name ) ;
	if( p == nullptr )
	{
		*n_out = 0 ;
		return 0 ;
	}

	size_t n = std::strlen( p ) ;
	*n_out = n + 1U ;

	if( n >= n_in )
		return ERANGE ;

	if( buffer )
	{
		for( ++n ; n ; n-- )
			*buffer++ = *p++ ;
	}

	return 0 ;
}
#endif

std::string G::Environment::get( const std::string & name , const std::string & default_ )
{
	std::size_t n = 0U ;
	errno_t rc = getenv_s( &n , nullptr , 0U , name.c_str() ) ;
	if( n == 0U ) // rc will be ERANGE if the environment variable exists
		return default_ ;

	std::vector<char> buffer( n ) ;
	rc = getenv_s( &n , &buffer[0] , n , name.c_str() ) ;
	if( rc != 0 || n == 0U )
		return default_ ;

	buffer.push_back( '\0' ) ; // just in case
	return std::string( &buffer[0] ) ;
}

G::Environment G::Environment::minimal( bool )
{
	return Environment() ;
}

void G::Environment::put( const std::string & name , const std::string & value )
{
	// dont use _putenv_s() here in order to maintain compatibility
	// with ancient run-times
	std::string s = name + "=" + value ;
	char * deliberately_leaky_copy = _strdup( s.c_str() ) ;
	GDEF_IGNORE_RETURN _putenv( deliberately_leaky_copy ) ;
}

