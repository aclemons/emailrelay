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
/// \file serverconfiguration.h
///

#ifndef G_MAIN_GUI_SERVER_CONFIGURATION_H
#define G_MAIN_GUI_SERVER_CONFIGURATION_H

#include "gdef.h"
#include "gstringarray.h"
#include "gmapfile.h"
#include "gpath.h"

//| \class ServerConfiguration
/// An interface for manipulating an emailrelay server configuration
/// taken from a configuration file, startup batch file or stack of gui pages.
///
class ServerConfiguration
{
public:
	explicit ServerConfiguration( const G::Path & ) ;
		///< Constructor that reads the emailrelay server configuration from a
		///< configuration file or startup batch file. If the file does
		///< not exist then the exe() and args() will be empty.

	static ServerConfiguration fromPages( const G::MapFile & pages_output ) ;
		///< Factory function using the output from the stack of
		///< gui pages (see GPage::dump()).

	static std::string exe( const G::Path & bat ) ;
		///< Returns the server executable path read from a startup batch file.
		///< Returns the empty string if the given path is not a windows
		///< batch file.

	G::StringArray args( bool disallow_close_stderr = false ) const ;
		///< Returns the list of emailrelay server command-line arguments.

	const G::MapFile & map() const ;
		///< Accessor for the underlying configuration map.

private:
	ServerConfiguration() ;
	static G::MapFile read( const G::Path & config_file ) ;
	static std::string quote( const std::string & ) ;

private:
	G::MapFile m_config ;
} ;

#endif
