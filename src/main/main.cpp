//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// main.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gstr.h"
#include "garg.h"
#include "run.h"
#include "options.h"
#include "commandline.h"
#include <exception>
#include <cstdlib>

namespace Main
{
	class App ;
}

/// \class Main::App
/// An implementation of the Main::Output abstract interface
/// for command-line output. An App instance is passed to Main::Run.
/// \see Main::Run
///
class Main::App : public Main::Output
{
	virtual void output( const std::string & text , bool e ) override ;
	virtual G::Options::Layout layout() const override ;
	virtual bool simpleOutput() const override ;
} ;

void Main::App::output( const std::string & text , bool e )
{
	std::ostream & s = e ? std::cerr : std::cout ;
	s << text << std::flush ;
}

G::Options::Layout Main::App::layout() const
{
	return G::Options::Layout( 38U ) ;
}

bool Main::App::simpleOutput() const
{
	return true ;
}

int main( int argc , char * argv [] )
{
	bool ok = false ;
	try
	{
		G::Arg arg( argc , argv ) ;
		Main::App app ;
		Main::Run run( app , arg , Main::Options::spec(false) ) ;
		run.configure() ;
		if( run.runnable() )
			run.run() ;
		ok = true ;
	}
	catch( std::exception & e )
	{
		std::cerr << G::Arg::prefix(argv) << ": error: " << e.what() << std::endl ;
	}
	catch(...)
	{
		std::cerr << G::Arg::prefix(argv) << ": unrecognised exception" << std::endl ;
	}
	return ok ? EXIT_SUCCESS : EXIT_FAILURE ;
}

