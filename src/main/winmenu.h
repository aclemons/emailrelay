//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file winmenu.h
///

#ifndef G_MAIN_WIN_MENU_H
#define G_MAIN_WIN_MENU_H

#include "gdef.h"
#include "gexception.h"
#include "gwinbase.h"

namespace Main
{
	class WinMenu ;
}

//| \class Main::WinMenu
/// Implements the small pop-up menu for the system tray icon.
///
class Main::WinMenu
{
public:
	G_EXCEPTION( Error , tx("menu error") ) ;

	explicit WinMenu( unsigned int resource_id ) ;
		///< Constructor.

	~WinMenu() ;
		///< Destructor.

	int popup( const GGui::WindowBase & w , bool foreground , bool with_open , bool with_close ) ;
		///< Opens the menu as a popup and returns when the
		///< mouse button is released.
		/// \see TrackPopupMenuEx()

	void update( bool with_open , bool with_close ) ;
		///< Updates the menu, even while popup() is
		///< still running.

public:
	WinMenu( const WinMenu & ) = delete ;
	WinMenu( WinMenu && ) = delete ;
	WinMenu & operator=( const WinMenu & ) = delete ;
	WinMenu & operator=( WinMenu && ) = delete ;

private:
	HMENU m_hmenu ;
	HMENU m_hmenu_popup ;
} ;

#endif

