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
/// \file gpump.cpp
///

#include "gdef.h"
#include "gpump.h"
#include "gcracker.h"
#include "gdialog.h"
#include "gstack.h"
#include "gscope.h"
#include "glog.h"
#include "gassert.h"

WPARAM GGui::Pump::m_run_id = 1U ;
std::string GGui::Pump::m_quit_reason ;

std::string GGui::Pump::run()
{
	return runImp( false , 0 , 0 , false ).second ;
}

std::string GGui::Pump::run( HWND idle_window , unsigned int idle_message )
{
	PostMessage( idle_window , idle_message , 0 , 0 ) ; // pump priming
	return runImp( true , idle_window , idle_message , false ).second ;
}

std::pair<bool,std::string> GGui::Pump::runToEmpty()
{
	return runImp( false , 0 , 0 , true ) ;
}

std::pair<bool,std::string> GGui::Pump::runToEmpty( HWND idle_window , unsigned int idle_message )
{
	PostMessage( idle_window , idle_message , 0 , 0 ) ; // pump priming
	return runImp( true , idle_window , idle_message , true ) ;
}

void GGui::Pump::quit( const std::string & reason )
{
	G_DEBUG( "GGui::Pump::quit: quit-reason=[" << reason << "] run-id=" << m_run_id ) ;
	m_quit_reason = reason ;
	PostMessage( 0 , Cracker::wm_quit() , m_run_id , 0 ) ; // not PostQuitMessage()
}

bool GGui::Pump::getMessage( MSG * msg_p , bool block )
{
	BOOL rc =
		block ?
			GetMessage( msg_p , HNULL , 0 , 0 ) :
			PeekMessage( msg_p , HNULL , 0 , 0 , PM_REMOVE ) ;
	return rc != -1 && rc != 0 ; // sic
}

std::pair<bool,std::string> GGui::Pump::runImp( bool send_idle_messages , HWND hwnd_idle ,
	unsigned int wm_idle , bool run_to_empty )
{
	G::ScopeExit _([&](){ m_run_id++ ; }) ; // enable quit() for this run or the next
	MSG msg ;
	bool block = false ;
	bool done_idling = false ;
	bool seen_quit = false ;
	for(;;)
	{
		bool got_message = getMessage( &msg , block ) ;
		if( got_message )
		{
			block = false ;
			if( msg.message == Cracker::wm_quit() ) // (our own quit message, not WM_QUIT)
			{
				G_DEBUG( "GGui::Pump::quit: mw_quit message: wparam=" << msg.wParam << " run-id=" << m_run_id ) ;
				if( msg.wParam == m_run_id )
					seen_quit = true ;
			}
			else if( Dialog::dialogMessage(msg) )
			{
				; // no-op
			}
			else if( Stack::stackMessage(msg) )
			{
				; // no-op -- see PropSheet_IsDialogMessage()
			}
			else
			{
				TranslateMessage( &msg ) ;
				DispatchMessage( &msg ) ;
			}
		}
		else if( run_to_empty && send_idle_messages )
		{
			if( done_idling )
				break ;
			if( sendIdle( hwnd_idle , wm_idle ) )
				done_idling = true ;
		}
		else if( send_idle_messages )
		{
			block = sendIdle( hwnd_idle , wm_idle ) ;
		}
		else if( run_to_empty )
		{
			break ;
		}
		else
		{
			block = true ; // empty, so block for the next one
		}

		if( seen_quit && !run_to_empty )
			break ;
	}
	std::string reason = m_quit_reason ;
	m_quit_reason.clear() ;
	return { seen_quit , reason } ;
}

bool GGui::Pump::sendIdle( HWND hwnd_idle , unsigned int wm_idle )
{
	G_ASSERT( hwnd_idle != 0 ) ;
	return 1 == SendMessage( hwnd_idle , wm_idle , 0 , 0 ) ;
}

