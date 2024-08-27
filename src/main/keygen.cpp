//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file keygen.cpp
///
// Uses mbedtls to generate a self-signed certificate, to be used for
// demonstration and testing purposes only.
//
// usage: emailrelay-keygen [<issuer/subject> [<output-file>]]
//
// The issuer/subject defaults to "CN=example.com".
//

#include "gdef.h"
#include "gssl_mbedtls_keygen.h"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

void open_( std::ofstream & f , const std::string & path ) { f.open( path.c_str() , std::ios_base::out ) ; }

int main( int argc , char * argv [] )
{
	std::string arg_prefix( argv[0] ? argv[0] : "" ) ;
	std::size_t pos = arg_prefix.find_last_of( "/\\" ) ;
	if( pos != std::string::npos && (pos+1U) < arg_prefix.size() ) arg_prefix = arg_prefix.substr( pos+1U ) ;
	try
	{
		std::string arg1 = argc > 1 ? std::string(argv[1]) : std::string() ;
		std::string arg2 = argc > 2 ? std::string(argv[2]) : std::string() ;
		if( ( !arg1.empty() && arg1.at(0) == '-' ) || arg1 == "/?" )
		{
			std::cout << "usage: " << arg_prefix << " [<issuer/subject> [<out-file>]]" << std::endl ;
			std::cout << "This program comes with ABSOLUTELY NO WARRANTY." << std::endl ;
			std::cout << "For demonstration and testing purposes only." << std::endl ;
			return 2 ;
		}

		std::string arg_name = arg1.empty() ? std::string("CN=example.com") : arg1  ;
		std::string arg_filename = arg2 ;

		std::string s = GSsl::MbedTls::generateKey( arg_name ) ;
		if( s.empty() )
			throw std::runtime_error( "not implemented: rebuild with mbedtls" ) ;

		std::ofstream file ;
		std::ostream & out = arg_filename.empty() ? static_cast<std::ostream&>(std::cout) : static_cast<std::ostream&>(file) ;
		if( !arg_filename.empty() )
		{
			open_( file , arg_filename ) ;
			if( !file.good() )
				throw std::runtime_error( "cannot create output file: " + arg_filename ) ;
			file.exceptions( std::ios_base::failbit | std::ios_base::badbit ) ;
		}

		out << s << std::flush ;

		if( !arg_filename.empty() )
			file.close() ;

		return 0 ;
	}
	catch( std::exception & e )
	{
		std::cerr << arg_prefix << ": error: " << e.what() << std::endl ;
	}
	catch(...)
	{
		std::cerr << arg_prefix << ": error" << std::endl ;
	}
	return 1 ;
}

