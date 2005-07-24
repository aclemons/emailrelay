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
// winmenu.h
//

#ifndef WIN_MENU_H
#define WIN_MENU_H

#include "gdef.h"
#include "gexception.h"
#include "gwinbase.h"

namespace Main
{
	class WinMenu ;
}

class Main::WinMenu 
{
public:
	G_EXCEPTION( Error , "menu error" ) ;

	explicit WinMenu( unsigned int resource_id ) ;
		// Constructor.

	~WinMenu() ;
		// Destructor.

	int popup( const GGui::WindowBase & w , bool with_open , bool with_close ) ;
		// Opens the menu as a popup.
		// See also: TrackPopupMenuEx()

private: 
	HMENU m_hmenu ;
	HMENU m_hmenu_popup ;
	WinMenu( const WinMenu & ) ; // not implemented
	void operator=( const WinMenu & ) ; // not implemented
} ;

#endif

