//
// Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// main.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gstr.h"
#include "garg.h"
#include "run.h"
#include "commandline.h"
#include <exception>

// Class: App
// Description: An implementation of the Main::Output abstract interface
// that can be passed to Main::Run.
// See also: Main::Run
//
struct App : public Main::Output
{
	void output( const std::string & text , bool e )
	{
		std::ostream & s = e ? std::cerr : std::cout ;
		s << text << std::flush ;
	}
	unsigned int columns()
	{
		const unsigned int default_ = 79U ;
		try 
		{ 
			const char * p = std::getenv( "COLUMNS" ) ;
			return p == NULL ? default_ : G::Str::toUInt(p) ;
		}
		catch( std::exception & )
		{
			return default_ ;
		}
	}
} ;

int main( int argc , char * argv [] )
{
	try
	{
		G::Arg arg( argc , argv ) ;
		App app ;
		Main::Run main( app , arg , Main::CommandLine::switchSpec(false) ) ;
		if( main.prepare() )
			main.run() ;
		return EXIT_SUCCESS ;
	}
	catch( std::exception & e )
	{
		std::cerr << G::Arg::prefix(argv) << ": exception: " << e.what() << std::endl ;
	}
	catch(...)
	{
		std::cerr << G::Arg::prefix(argv) << ": unrecognised exception" << std::endl ;
	}
	return EXIT_FAILURE ;
}

