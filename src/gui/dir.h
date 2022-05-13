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
/// \file dir.h
///

#ifndef G_MAIN_GUI_DIR_H
#define G_MAIN_GUI_DIR_H

#include "gdef.h"
#include "gpath.h"
#include <string>
#include <iostream>

//| \class Dir
/// Provides default file-system paths. The implementations
/// of this interface are highly platform-specific.
///
class Dir
{
public:
	static G::Path install() ;
		///< Returns the default install directory, eg. "/usr".

	static G::Path config() ;
		///< Returns the configuration directory path, eg. "/etc".

	static G::Path spool() ;
		///< Returns the spool directory path, eg. "/var/spool".

	static G::Path boot() ;
		///< Returns the default boot-time autostart directory path, "/etc/init.d".

	static G::Path home() ;
		///< Returns the user's home directory, eg. "/home/username".

	static G::Path pid( const G::Path & config_dir ) ;
		///< Returns the directory for pid files, eg. "/run".

	static G::Path desktop() ;
		///< Returns the desktop path, eg. "/home/username/Desktop".

	static G::Path autostart() ;
		///< Returns the autostart directory path, eg. "/home/username/AutoStart".

	static G::Path menu() ;
		///< Returns the menu path, eg. "/home/username/.local/share/applications".

public:
	Dir() = delete ;

private:
	static G::Path envPath( const std::string & , const G::Path & = G::Path() ) ;
	static bool ok( const std::string & ) ;
	static std::string rebase( const std::string & ) ;
	static G::Path oneOf( std::string , std::string = {} , std::string = {} ,
		std::string = {} , std::string = {} ) ;
	static G::Path os_install() ;
	static G::Path os_boot() ;
	static G::Path os_config() ;
	static G::Path special( const std::string & key ) ;
	static G::Path os_pid( const G::Path & ) ;
	static G::Path os_spool() ;
} ;

#endif
