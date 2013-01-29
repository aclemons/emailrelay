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
///
/// \file dir.h
///

#ifndef G_GUI_DIR_H
#define G_GUI_DIR_H

#include "gdef.h"
#include "gpath.h"
#include <string>
#include <iostream>

/// \class Dir
/// Provides default file-system paths. The implementations 
/// of this interface are highly platform-specific.
///
class Dir 
{
public:

	static G::Path install() ;
		///< Returns the default install directory. This is what the user 
		///< thinks of as the install point, and not strictly related to the 
		///< "make install" process.

	static G::Path config() ;
		///< Returns the configuration directory path.

	static G::Path boot() ;
		///< Returns the default boot-time autostart directory path.

	static G::Path gui( const G::Path & install ) ;
		///< Returns the full path of the GUI program for 
		///< a given install root.

	static G::Path icon( const G::Path & install ) ;
		///< Returns the full path of the icon file for 
		///< a given install root.

	static G::Path server( const G::Path & install ) ;
		///< Returns the full path of the main server program for 
		///< a given install root.

	static G::Path bootcopy( const G::Path & boot , const G::Path & install ) ;
		///< Returns a directory where boot() files can be stored if boot() is 
		///< not writeable.
		///<
		///< Copying the boot() files allows the install to complete without 
		///< root privileges; the user can fix up the boot process later
		///< by installing the copies (especially on a mac).

	static G::Path home() ;
		///< Returns the user's home directory.

	static G::Path spool() ;
		///< Returns the spool directory path.

	static G::Path pid( const G::Path & config_dir ) ;
		///< Returns the directory for pid files.

	static G::Path cwd() ;
		///< Returns the current working directory.

	static G::Path thisdir( const std::string & argv0 , const G::Path & initial_cwd ) ;
		///< Returns the argv0 directory as an absolute path.

	static G::Path thisexe( const std::string & argv0 , const G::Path & initial_cwd ) ;
		///< Returns the argv0 path.

	static G::Path desktop() ;
		///< Returns the desktop path.

	static G::Path login() ;
		///< Returns the login autostart directory path.

	static G::Path menu() ;
		///< Returns the menu path.

	static std::string dotexe() ;
		///< Returns ".exe" or not.

private:
	Dir() ;
	static G::Path windows() ;
	static std::string env( const std::string & , const std::string & = std::string() ) ;
	static G::Path envPath( const std::string & , const G::Path & = G::Path() ) ;
	static bool ok( const std::string & ) ;
	static G::Path oneOf( std::string , std::string = std::string() , std::string = std::string() , 
		std::string = std::string() , std::string = std::string() ) ;
	static G::Path os_install() ;
	static G::Path os_gui( const G::Path & ) ;
	static G::Path os_icon( const G::Path & ) ;
	static G::Path os_server( const G::Path & ) ;
	static G::Path os_bootcopy( const G::Path & , const G::Path & ) ;
	static G::Path os_boot() ;
	static G::Path os_config() ;
	static G::Path special( const std::string & key ) ;
	static G::Path os_pid( const G::Path & ) ;
	static G::Path os_spool() ;
} ;

#endif

