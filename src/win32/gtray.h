//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gtray.h
//

#ifndef G_GUI_TRAY_H
#define G_GUI_TRAY_H

#include "gdef.h"
#include "gwinbase.h"
#include "gcracker.h"
#include "gexception.h"

namespace GGui
{
	class Tray ;
} ;

// Class: GGui::Tray
// Description: Manages an icon within the system tray.
//
class GGui::Tray 
{
public:
	G_EXCEPTION( Error , "system-tray error" ) ;

	Tray( unsigned int icon_resource_id , const WindowBase & window ,
		const std::string & tip , unsigned int message = Cracker::wm_tray() ) ;
			// Constructor. Adds the icon to the system tray.

	~Tray() ;
		// Destructor. Removes the icon from the system tray.

private:
	void operator=( const Tray & ) ; // not implemented
	Tray( const Tray & ) ; // not implemented

private:
	NOTIFYICONDATA m_info ;
} ;

#endif

