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
/// \file admin.h
///

#ifndef G_MAIN_ADMIN_H
#define G_MAIN_ADMIN_H

#include "gdef.h"
#include "gsmtp.h"
#include "gsmtpclient.h"
#include "gsecrets.h"
#include "gadminserver.h"
#include "configuration.h"
#include <memory>
#include <string>

/// \namespace Main
namespace Main
{
	class Admin ;
}

/// \class Main::Admin
/// A factory class for creating GSmtp::AdminServer objects.
///
class Main::Admin 
{
public:
	static bool enabled() ;
		///< Returns true if newServer() is fully implemented.

	static std::auto_ptr<GSmtp::AdminServer> newServer( const Configuration & , 
		GSmtp::MessageStore & store , const GSmtp::Client::Config & , 
		const GAuth::Secrets & client_secrets , const std::string & version_number ) ;
			///< A factory function for creating a new GSmtp::AdminServer
			///< instance on the heap.

	static void notify( GSmtp::AdminServer & server , const std::string & , const std::string & , const std::string & );
		///< Calls notify() on the given server.

	static void report( const GSmtp::AdminServer & server ) ;
		///< Calls report() on the given server.

private:
	Admin() ; // not implemented
} ;

#endif
