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
/// \file gtray.h
///

#ifndef G_GUI_TRAY_H
#define G_GUI_TRAY_H

#include "gdef.h"
#include "gnowide.h"
#include "gwinbase.h"
#include "gcracker.h"
#include "gexception.h"

namespace GGui
{
	class Tray ;
}

//| \class GGui::Tray
/// Manages an icon within the system tray.
///
class GGui::Tray
{
public:
	G_EXCEPTION( IconError , tx("no icon resource built-in") )
	G_EXCEPTION( Error , tx("system-tray error") )

	Tray( unsigned int icon_resource_id , const WindowBase & window ,
		const std::string & tip , unsigned int message = Cracker::wm_tray() ) ;
			///< Constructor. Adds the icon to the system tray.
			///< Notification messages are sent to the given
			///< window and the Cracker class converts them to
			///< onTrayDoubleClick(), onTrayRightMouseButtonUp(),
			///< onTrayRightMouseButtonDown(), and
			///< onTrayLeftMouseButtonDown().

	~Tray() ;
		///< Destructor. Removes the icon from the system tray.

public:
	Tray & operator=( const Tray & ) = delete ;
	Tray & operator=( Tray && ) = delete ;
	Tray( const Tray & ) = delete ;
	Tray( Tray && ) = delete ;

private:
	G::nowide::NOTIFYICONDATA_type m_info ;
} ;

#endif

