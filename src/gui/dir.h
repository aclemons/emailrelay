//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// dir.h
//

#ifndef G_GUI_DIR_H
#define G_GUI_DIR_H

#include "gpath.h"
#include <string>

// Class: Dir
// Description: Provides file-system paths.
//
class Dir 
{
public:
	Dir( const std::string & argv0 , const std::string & prefix ) ;
		// Constructor.

	~Dir() ;
		// Destructor.

	static G::Path install() ;
		// Returns the installation path.

	static G::Path spool() ;
		// Returns the spool directory path.

	static G::Path config() ;
		// Returns the configuration directory path.

	static G::Path startup() ;
		// Returns the system startup directory (eg. "/etc/init.d").

	static G::Path pid() ;
		// Returns the directory for pid files.

	static G::Path cwd() ;
		// Returns the current working directory.

	static G::Path tooldir() ;
		// Returns the tool's directory.

	static G::Path thisdir() ;
		// Returns the argv0 directory as an absolute path.

	static G::Path thisexe() ;
		// Returns the argv0 path.

	static G::Path tmp() ;
		// Returns a writable directory for temporary files.
		// Returns thisdir() as long as it is found to be a 
		// writeable directory.

	static G::Path desktop() ;
		// Returns the desktop path.

	static G::Path login() ;
		// Returns the login autostart directory path.

	static G::Path boot() ;
		// Returns the boot-time autostart directory path.

	static G::Path menu() ;
		// Returns the menu path.

	static G::Path reskit() ;
		// Returns the windows resource kit path.

	static std::string dotexe() ;
		// Returns ".exe" or not.

private:
	static G::Path special( const std::string & key ) ;
		// Returns a special directory. The key is one 
		// of "desktop", "menu", "boot", "login", etc.

private:
	Dir() ;
	static Dir * instance() ;
	static G::Path windows() ;
	static std::string env( const std::string & , const std::string & = std::string() ) ;
	static G::Path oneOf( std::string , std::string = std::string() , std::string = std::string() , 
		std::string = std::string() , std::string = std::string() ) ;
	static G::Path prefix( G::Path ) ;
	static G::Path prefix( std::string ) ;

private:
	std::string m_argv0 ;
	std::string m_prefix ;
	static Dir * m_this ;
} ;

#endif

