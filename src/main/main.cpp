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
/// \file main.cpp
///

#include "gdef.h"
#include "gstr.h"
#include "garg.h"
#include "run.h"
#include "options.h"
#include "submission.h"
#include "commandline.h"
#include "gsocket.h"
#include <exception>
#include <cstdlib>

namespace Main
{
	class App ;
}

class Main::App : public Main::Output
{
private: // overrides
	void output( const std::string & text , bool e , bool ) override ;
	G::OptionsUsage::Config outputLayout( bool verbose ) const override ;
	bool outputSimple() const override ;
} ;

void Main::App::output( const std::string & text , bool e , bool )
{
	std::ostream & s = e ? std::cerr : std::cout ;
	s << text << std::flush ;
}

G::OptionsUsage::Config Main::App::outputLayout( bool ) const
{
	return {} ;
}

bool Main::App::outputSimple() const
{
	return true ;
}

int main( int argc , char * argv [] )
{
	bool ok = false ;
	try
	{
		G::Arg arg = G::is_windows() ? G::Arg::windows() : G::Arg(argc,argv) ;

		#if GCONFIG_ENABLE_SUBMISSION
			if( Main::Submission::enabled(arg) )
				return Main::Submission::submit( arg ) ;
		#endif

		Main::App app ;
		Main::Run run( app , arg ) ;
		run.configure( Main::Options::spec() ) ;
		if( run.runnable() )
		{
			run.run() ;
			ok = true ;
		}
	}
	catch( GNet::SocketBase::SocketBindError & e )
	{
		std::cerr << G::Arg::prefix(argv) << ": error: " << e.what() << std::endl ;
		return 2 ;
	}
	catch( std::exception & e )
	{
		std::cerr << G::Arg::prefix(argv) << ": error: " << e.what() << std::endl ;
		return 1 ;
	}
	catch(...)
	{
		std::cerr << G::Arg::prefix(argv) << ": fatal exception" << std::endl ;
		return 1 ;
	}
	return ok ? 0 : 1 ;
}

