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
/// \file servicecontrol.h
///

#ifndef G_MAIN_SERVICE_CONTROL_H
#define G_MAIN_SERVICE_CONTROL_H

#include "gdef.h"
#include <string>

std::string service_install( const std::string & commandline , const std::string & name ,
	const std::string & display_name_latin1 , const std::string & description_latin1 ,
	bool autostart = true ) ;

bool service_installed( const std::string & name ) ;

std::string service_remove( const std::string & name ) ;

std::string service_start( const std::string & name ) ;

#endif
