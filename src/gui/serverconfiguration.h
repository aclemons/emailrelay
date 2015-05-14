//
// Copyright (C) 2001-2015 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef SERVER_CONFIGURATION_H__
#define SERVER_CONFIGURATION_H__

#include "gdef.h"
#include "gstrings.h"
#include "gmapfile.h"
#include "gpath.h"

/// \class ServerConfiguration
/// An interface for manipulating an emailrelay server configuration.
/// 
class ServerConfiguration
{
public:
	explicit ServerConfiguration( const G::Path & ) ;
		///< Constructor that reads the emailrelay server configuration from a 
		///< configuration file or startup batch file.

	static ServerConfiguration fromPages( const G::MapFile & pages_output , const G::Path & copy_filter ) ;
		///< Factory function using the output from the stack of
		///< gui pages (see GPage::dump()).

	static std::string exe( const G::Path & ) ;
		///< Returns the server executable path read from a startup batch file.

	G::StringArray args( bool disallow_close_stderr = false ) const ;
		///< Returns the list of emailrelay server command-line arguments.

	const G::MapFile & map() const ;
		///< Accessor for the underlying configuration map.

private:
	ServerConfiguration() ;
	static G::MapFile read( const G::Path & config_file ) ;
	static std::string spec() ;
	static std::string quote( std::string , bool ) ;

private:
	G::MapFile m_config ;
} ;

#endif
