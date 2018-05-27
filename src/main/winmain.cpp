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
// winmain.cpp
//

#include "gdef.h"
#include "garg.h"
#include "gslot.h"
#include "gexception.h"
#include "winapp.h"
#include "commandline.h"
#include "options.h"
#include "run.h"
#include "resource.h"
#include <clocale>

int WINAPI WinMain( HINSTANCE hinstance , HINSTANCE previous , LPSTR command_line , int show_style )
{
	try
	{
		::setlocale( LC_ALL , "" ) ; // set the locale from the environment

		G::Arg arg ;
		arg.parse( hinstance , command_line ) ;

		Main::WinApp app( hinstance , previous , "E-MailRelay" ) ;
		Main::Run run( app , arg , Main::Options::spec(true) ) ;
		try
		{
			run.configure() ;
			if( run.hidden() )
				app.disableOutput() ;

			if( run.runnable() )
			{
				app.init( run.configuration() ) ;
				app.createWindow( show_style , /*show*/false ) ; // invisible main window
				run.signal().connect( G::Slot::slot(app,&Main::WinApp::onRunEvent) ) ;
				run.run() ;
			}
		}
		catch( std::exception & e )
		{
			app.onError( e.what() ) ;
			return 1 ;
		}

		return app.exitCode() ;
	}
	catch(...)
	{
		::MessageBeep( MB_ICONHAND ) ;
	}
	return 1 ;
}

/// \file winmain.cpp
