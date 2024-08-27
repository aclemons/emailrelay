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
/// \file winmain.cpp
///

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

int WINAPI WinMain( HINSTANCE hinstance , HINSTANCE previous , LPSTR /*command_line*/ , int show_style )
{
	try
	{
		#if 0
			// set the C locale from the environment -- this has no effect
			// since we avoid things like atoi(), tolower(), strtoul() etc.
			::setlocale( LC_ALL , "" ) ;
		#endif

		G::Arg arg = G::Arg::windows() ; // GetCommandLineW()

		Main::WinApp app( hinstance , previous , "E-MailRelay" ) ;
		Main::Run run( app , arg , /*has-gui=*/true ) ;
		try
		{
			G::Options options_spec = Main::Options::spec() ;
			run.configure( options_spec ) ;
			if( run.hidden() )
				app.disableOutput() ;

			if( run.runnable() )
			{
				app.init( run.configuration(0U) , options_spec ) ;
				app.createWindow( show_style , /*show=*/false , 10 , 10 ) ; // main window, not shown
				run.signal().connect( G::Slot::slot(app,&Main::WinApp::onRunEvent) ) ;
				run.run() ;
			}
		}
		catch( GNet::SocketBase::SocketBindError & e )
		{
			if( e.m_einuse )
			{
				std::string_view help = "check whether emailrelay is already running as a service" ;
				app.onError( std::string(e.what()).append(": ",2U).append(help.data(),help.size()) , 2 ) ;
			}
			else
			{
				app.onError( e.what() , 2 ) ;
			}
		}
		catch( std::exception & e )
		{
			app.onError( e.what() , 1 ) ;
		}

		return app.exitCode() ;
	}
	catch(...)
	{
		MessageBeep( MB_ICONHAND ) ;
	}
	return 1 ;
}

