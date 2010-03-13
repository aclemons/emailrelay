//
// Copyright (C) 2001-2010 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#include "gpath.h"
#include "state.h"
#include <string>
#include <iostream>

/// \class Dir
/// Provides file-system paths. Note that some paths
/// returned by this interface are used as defaults; the run-time 
/// values in client code can be something different.
///
/// The implementations of this interface are highly platform-specific.
///
class Dir 
{
public:
	explicit Dir( const std::string & argv0 ) ;
		///< Constructor.

	void read( const State & ) ;
		///< Reads from a state file.

	~Dir() ;
		///< Destructor.

	static G::Path install() ;
		///< Returns the default install directory. This
		///< is what the user thinks of as the install 
		///< point, and not strictly related to the 
		///< "make install" process.

	static G::Path config( int ) ;
		///< Returns the default configuration directory path.

	G::Path config() const ;
		///< Returns the configuration directory path.

	static G::Path boot( int ) ;
		///< Returns the default boot-time autostart directory path.

	G::Path boot() const ;
		///< Returns the boot-time autostart directory path.

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
		///< Returns a directory where boot() files can
		///< be stored if boot() is not writeable.
		///<
		///< Copying the boot() files allows the install 
		///< to complete without root privileges; the
		///< user can fix up the boot process later
		///< by installing the copies (especially on a
		///< mac).

	static G::Path home() ;
		///< Returns the user's home directory.

	G::Path spool() const ;
		///< Returns the spool directory path.

	G::Path pid( const G::Path & config_dir ) const ;
		///< Returns the directory for pid files.

	static G::Path cwd() ;
		///< Returns the current working directory.

	G::Path thisdir() const ;
		///< Returns the argv0 directory as an absolute path.

	G::Path thisexe() const ;
		///< Returns the argv0 path.

	G::Path desktop() const ;
		///< Returns the desktop path.

	G::Path login() const ;
		///< Returns the login autostart directory path.

	G::Path menu() const ;
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
	static G::Path ntspecial( const std::string & key ) ;
	static G::Path os_pid() ;
	static G::Path os_pid( const G::Path & , const G::Path & ) ;
	G::Path os_spool() const ;
	G::Path os_login() const ;

private:
	G::Path m_spool ;
	G::Path m_config ;
	G::Path m_login ;
	G::Path m_pid ;
	G::Path m_thisdir ;
	G::Path m_thisexe ;
	G::Path m_desktop ;
	G::Path m_boot ;
	G::Path m_menu ;
} ;

#endif

