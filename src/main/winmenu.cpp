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
/// \file winmenu.cpp
///

#include "gdef.h"
#include "gnowide.h"
#include "gappinst.h"
#include "winmenu.h"
#include "glog.h"

namespace Main
{
	const int open_pos = 0 ;
	const int close_pos = 1 ;
	struct ScopeZero
	{
		ScopeZero( HMENU h ) : m_h(h) {}
		~ScopeZero() { m_h = 0 ; }
		HMENU & m_h ;
	} ;
}

Main::WinMenu::WinMenu( unsigned int id ) :
	m_hmenu_popup(0)
{
	HINSTANCE hinstance = GGui::ApplicationInstance::hinstance() ;
	m_hmenu = G::nowide::loadMenu( hinstance , id ) ;
	if( m_hmenu == nullptr )
		throw Error() ;
}

int Main::WinMenu::popup( const GGui::WindowBase & w , bool set_foreground , bool with_open , bool with_close )
{
	POINT p ;
	GetCursorPos( &p ) ;
	if( set_foreground )
		SetForegroundWindow( w.handle() ) ;

	// TrackPopup() only works with a sub-menu
	//
	ScopeZero _( m_hmenu_popup ) ;
	m_hmenu_popup = GetSubMenu( m_hmenu , 0 ) ;

	// make the "open" menu item bold
	//
	SetMenuDefaultItem( m_hmenu_popup , open_pos , TRUE ) ;

	// optionally grey-out menu items
	//
	if( !with_open )
		EnableMenuItem( m_hmenu_popup , open_pos , MF_BYPOSITION | MF_GRAYED ) ;
	if( !with_close )
		EnableMenuItem( m_hmenu_popup , close_pos , MF_BYPOSITION | MF_GRAYED ) ;

	// display the menu
	//
	G_DEBUG( "Main::WinMenu::popup: tracking start" ) ;
	int rc = static_cast<int>( TrackPopupMenuEx( m_hmenu_popup , TPM_RETURNCMD ,
		p.x , p.y , w.handle() , nullptr ) ) ;
	G_DEBUG( "Main::WinMenu::popup: tracking end: " << rc ) ;

	// (from the TrackPopupMenu() documentation (not TrackPopupMenuEx()))
	G::nowide::postMessage( w.handle() , WM_NULL , 0 , 0 ) ;

	return rc ;
}

void Main::WinMenu::update( bool with_open , bool with_close )
{
	G_DEBUG( "Main::WinMenu::update: with-open=" << with_open << " "
		<< "with-close=" << with_close << " hmenu=" << m_hmenu_popup ) ;
	if( m_hmenu_popup )
	{
		EnableMenuItem( m_hmenu_popup , open_pos , MF_BYPOSITION | (with_open?0:MF_GRAYED) ) ;
		EnableMenuItem( m_hmenu_popup , close_pos , MF_BYPOSITION | (with_close?0:MF_GRAYED) ) ;
	}
}

Main::WinMenu::~WinMenu()
{
	if( m_hmenu != nullptr )
		DestroyMenu( m_hmenu ) ;
}

