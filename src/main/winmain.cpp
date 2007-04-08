//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// winmain.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "garg.h"
#include "gslot.h"
#include "gexception.h"
#include "winapp.h"
#include "commandline.h"
#include "run.h"
#include <clocale>

int WINAPI WinMain( HINSTANCE hinstance , HINSTANCE previous , 
	LPSTR command_line , int show )
{
	try
	{
		::setlocale( LC_ALL , "" ) ;

		G::Arg arg ;
		arg.parse( hinstance , command_line ) ;
		Main::WinApp app( hinstance , previous , "E-MailRelay" ) ;

		try
		{
			Main::Run run( app , arg , Main::CommandLine::switchSpec(true) ) ;
			if( run.prepare() )
			{
				const bool visible = ! run.cfg().daemon() ;
				app.init( run.cfg() ) ;
				app.createWindow( show , visible ) ;
				run.signal().connect( G::slot(app,&Main::WinApp::onRunEvent) ) ;
				run.run() ;
			}
		}
		catch( std::exception & e )
		{
			app.onError( e.what() ) ;
		}

		return 0 ;
	}
	catch(...)
	{
		::MessageBeep( MB_ICONHAND ) ;
	}
	return 1 ;
}


/// \file winmain.cpp
