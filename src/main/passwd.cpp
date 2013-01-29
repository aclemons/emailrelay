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
// passwd.cpp
//
// A utility which encrypts a password so that it can be pasted 
// into the emailrelay secrets file(s) and used for CRAM-MD5
// authentication.
//
// The password must be supplied on the standard input so that it
// is not visible in the command-line history.
//

#include "gdef.h"
#include "gstr.h"
#include "garg.h"
#include "gmd5.h"
#include "legal.h"
#include <iostream>
#include <cstdlib>

int main( int argc , char * argv [] )
{
	G::Arg arg( argc , argv ) ;
	try
	{
		if( argc != 1 )
		{
			std::cerr 
				<< arg.prefix() << ": too many command-line arguments "
					"(the password is read from the standard input)" << std::endl
				<< "usage: " << arg.prefix() << std::endl
				<< std::endl
				<< Main::Legal::warranty("  ","\n")
				<< "    " << Main::Legal::copyright() << std::endl ;
			return EXIT_FAILURE ;
		}

		std::string key = G::Str::readLineFrom( std::cin ) ;
		G::Str::trim( key , " \t\n\r" ) ;
		if( key.length() == 0U )
		{
			std::cerr << arg.prefix() << ": invalid password" << std::endl ;
			return EXIT_FAILURE ;
		}

		std::cout << G::Md5::mask(key) << std::endl ;
		return EXIT_SUCCESS ;
	}
	catch( std::exception & e )
	{
		std::cerr << arg.prefix() << ": exception: " << e.what() << std::endl ;
	}
	catch(...)
	{
		std::cerr << arg.prefix() << ": unknown exception" << std::endl ;
	}
	return EXIT_FAILURE ;
}

/// \file passwd.cpp
