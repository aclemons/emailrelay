//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gpump.cpp
//

#include "gdef.h"
#include "gpump.h"
#include "gcracker.h"
#include "gdebug.h"

void GGui::Pump::run()
{
	runCore( false , 0 , 0 ) ;
}

void GGui::Pump::run( HWND idle_window , unsigned int idle_message )
{
	G_LOG( "GGui::Pump::run: " << idle_window << " " << idle_message ) ;
	::PostMessage( idle_window , idle_message , 0 , 0 ) ; // pump priming
	runCore( true , idle_window , idle_message ) ;
}

void GGui::Pump::runCore( bool idle , HWND hwnd_idle , unsigned int wm_idle )
{
	MSG msg ;
	while( ::GetMessage( &msg , NULL , 0 , 0 ) )
	{
		// we use our own quit message rather than WM_QUIT because
		// WM_QUIT has some nasty undocumented side-effects such as 
		// making message boxes invisible -- we need to quit event
		// loops (sometimes more than once) without side effects
		//
		if( msg.message == Cracker::wm_quit() )
		{
			break ;
		}
		else if( dialogMessage( msg ) ) // (may be stubbed out as a build option)
		{
			; // no-op
		}
		else
		{
			::TranslateMessage( &msg ) ;
			::DispatchMessage( &msg ) ;
		}

		while( idle && empty() && sendIdle(hwnd_idle,wm_idle) )
		{
			; // no-op
		}
	}
}

bool GGui::Pump::empty()
{
	MSG msg ;
	bool message = !! ::PeekMessage( &msg , NULL , 0 , 0 , PM_NOREMOVE ) ;
	return ! message ;
}

bool GGui::Pump::sendIdle( HWND hwnd_idle , unsigned int wm_idle )
{
	G_ASSERT( hwnd_idle != 0 ) ;
	return !! ::SendMessage( hwnd_idle , wm_idle , 0 , 0 ) ;
}

void GGui::Pump::quit()
{
	::PostMessage( 0 , Cracker::wm_quit() , 0 , 0 ) ; // not PostQuitMessage()
}

