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
/// \file servicecontrol.h
///

#ifndef G_MAIN_SERVICE_CONTROL_H
#define G_MAIN_SERVICE_CONTROL_H

#include "gdef.h"
#include <string>
#include <utility>

// this interface is used by the GUI installer via Gui::Boot and by the
// service wrapper via ServiceImp (for its "--install" and "--remove"
// options) -- the non-Windows implementations do nothing

std::pair<std::string,DWORD> service_install( const std::string & commandline , const std::string & name ,
	const std::string & display_name , const std::string & description ,
	bool autostart = true ) ;

bool service_installed( const std::string & name ) ;

std::pair<std::string,DWORD> service_remove( const std::string & name ) ;

std::pair<std::string,DWORD> service_start( const std::string & name ) ;

#endif
