//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include <string>

/// \class Dir
/// Provides file-system paths.
///
class Dir 
{
public:
	Dir( const std::string & argv0 , bool installed ) ;
		///< Constructor.

	void read( std::istream & ) ;
		///< Reads from a state file.

	~Dir() ;
		///< Destructor.

	G::Path install() const ;
		///< Returns the installation path.

	G::Path spool() const ;
		///< Returns the spool directory path.

	G::Path config() const ;
		///< Returns the configuration directory path.

	G::Path pid() const ;
		///< Returns the directory for pid files.

	static G::Path cwd() ;
		///< Returns the current working directory.

	G::Path thisdir() const ;
		///< Returns the argv0 directory as an absolute path.

	G::Path thisexe() const ;
		///< Returns the argv0 path.

	G::Path tmp() const ;
		///< Returns a writable directory for temporary files.
		///< Returns thisdir() as long as it is found to be a 
		///< writeable directory.

	G::Path desktop() const ;
		///< Returns the desktop path.

	G::Path login() const ;
		///< Returns the login autostart directory path.

	G::Path boot() const ;
		///< Returns the boot-time autostart directory path.

	G::Path menu() const ;
		///< Returns the menu path.

	G::Path reskit() const ;
		///< Returns the windows resource kit path.

	static std::string dotexe() ;
		///< Returns ".exe" or not.

private:
	Dir() ;
	static G::Path windows() ;
	static std::string env( const std::string & , const std::string & = std::string() ) ;
	static bool ok( const std::string & ) ;
	static G::Path oneOf( std::string , std::string = std::string() , std::string = std::string() , 
		std::string = std::string() , std::string = std::string() ) ;
	G::Path os_install() const ;
	G::Path os_config() const ;
	G::Path os_spool() const ;
	G::Path os_login() const ;
	G::Path os_pid() const ;
	G::Path os_boot() const ;
	static G::Path special( const std::string & key ) ;
	static G::Path ntspecial( const std::string & key ) ;

private:
	std::string m_argv0 ;
	G::Path m_install ;
	G::Path m_spool ;
	G::Path m_config ;
	G::Path m_login ;
	G::Path m_pid ;
	G::Path m_thisdir ;
	G::Path m_thisexe ;
	G::Path m_tmp ;
	G::Path m_desktop ;
	G::Path m_boot ;
	G::Path m_menu ;
	G::Path m_reskit ;
} ;

#endif

