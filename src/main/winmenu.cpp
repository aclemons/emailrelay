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
// winmenu.cpp
//

#include "gdef.h"
#include "gappinst.h"
#include "winmenu.h"

Main::WinMenu::WinMenu( unsigned int id )
{
	HINSTANCE hinstance = GGui::ApplicationInstance::hinstance() ;
	m_hmenu = ::LoadMenu( hinstance , MAKEINTRESOURCE(id) ) ;
	if( m_hmenu == NULL )
		throw Error() ;
}

int Main::WinMenu::popup( const GGui::WindowBase & w , bool with_open , bool with_close )
{
	const int open_pos = 0 ;
	const int close_pos = 1 ;

	POINT p ;
	::GetCursorPos( &p ) ;
	::SetForegroundWindow( w.handle() ) ;

	// TrackPopup() only works with a sub-menu, although
	// you would never guess from the documentation
	//
	m_hmenu_popup = ::GetSubMenu( m_hmenu , 0 ) ;

	// make the "open" menu item bold
	// 
	::SetMenuDefaultItem( m_hmenu_popup , open_pos , TRUE ) ;

	// optionally grey-out menu items
	//
	if( !with_open )
		::EnableMenuItem( m_hmenu_popup , open_pos , MF_BYPOSITION | MF_GRAYED ) ;
	if( !with_close )
		::EnableMenuItem( m_hmenu_popup , close_pos , MF_BYPOSITION | MF_GRAYED ) ;

	// display the menu
	//
	BOOL rc = ::TrackPopupMenuEx( m_hmenu_popup , 
		TPM_RETURNCMD , p.x , p.y , w.handle() , NULL ) ;
	return static_cast<int>(rc) ; // BOOL->int!, only in Microsoft wonderland
}

Main::WinMenu::~WinMenu()
{
	if( m_hmenu != NULL )
		::DestroyMenu( m_hmenu ) ;
}

/// \file winmenu.cpp
