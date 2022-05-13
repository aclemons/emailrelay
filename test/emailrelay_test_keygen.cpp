//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// usage: keygen [<issuer/subject> [<output-file>]]
//
// The issuer/subject defaults to "CN=example.com".
//
// See:
// * https://tls.mbed.org/kb/how-to/generate-a-self-signed-certificate
// * mbedtls/programs/pkey/gen_key.c
// * mbedtls/programs/x509/cert_write.c
//

#include "gdef.h"
#include "gssl.h"
#include "gfile.h"
#include "garg.h"
#include "gexception.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

int main( int argc , char * argv [] )
{
	try
	{
		G::Arg arg( argc , argv ) ;
		if( arg.c() > 1U && ( ( !arg.v(1).empty() && arg.v(1).at(0) == '-' ) || arg.v(1) == "/?" ) )
		{
			std::cout << "usage: " << arg.prefix() << " [<issuer/subject> [<out-file>]]" << std::endl ;
			std::cout << "This program comes with ABSOLUTELY NO WARRANTY." << std::endl ;
			std::cout << "For demonstration and testing purposes only." << std::endl ;
			return 2 ;
		}
		std::string arg_name = arg.v( 1 , "" ) ;
		if( arg_name.empty() ) arg_name = "CN=example.com" ;
		std::string arg_filename = arg.v( 2 , "" ) ;

		GSsl::Library ssl( true , "mbedtls,ignoreextra" ) ;
		std::string s = ssl.generateKey( arg_name ) ;
		if( s.empty() )
			throw G::Exception( "not implemented: rebuild with mbedtls" ) ;

		std::ofstream file ;
		std::ostream & out = arg_filename.empty() ? static_cast<std::ostream&>(std::cout) : static_cast<std::ostream&>(file) ;
		if( !arg_filename.empty() )
		{
			G::File::open( file , arg_filename , G::File::Text() ) ;
			if( !file.good() )
				throw G::Exception( "cannot create output file" , arg_filename ) ;
			file.exceptions( std::ios_base::failbit | std::ios_base::badbit ) ;
		}

		out << s << std::flush ;

		if( !arg_filename.empty() )
			file.close() ;

		return 0 ;
	}
	catch( std::exception & e )
	{
		std::cerr << G::Arg::prefix(argv) << ": error: " << e.what() << std::endl ;
	}
	catch(...)
	{
		std::cerr << G::Arg::prefix(argv) << ": error" << std::endl ;
	}
	return 1 ;
}

