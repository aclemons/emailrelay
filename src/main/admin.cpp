//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
//
// admin.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "admin.h"
#include "legal.h"
#include "gssl.h"
#include "gprocess.h"

bool Main::Admin::enabled()
{
	return true ;
}

unique_ptr<GSmtp::AdminServer> Main::Admin::newServer( GNet::ExceptionHandler & eh ,
	const Configuration & cfg , GSmtp::MessageStore & store ,
	const GSmtp::Client::Config & client_config , const GAuth::Secrets & client_secrets ,
	const std::string & version_number )
{
	G::StringMap info_map ;
	info_map["version"] = version_number ;
	info_map["warranty"] = Legal::warranty("","\n") ;
	info_map["credit"] = GSsl::Library::credit("","\n","") ;
	info_map["copyright"] = Legal::copyright() ;

	G::StringMap config_map ;
	//config_map["forward-to"] = cfg.serverAddress() ;
	//config_map["spool-dir"] = cfg.spoolDir().str() ;

	return unique_ptr<GSmtp::AdminServer>( new GSmtp::AdminServer(
			eh ,
			store ,
			client_config ,
			client_secrets ,
			GNet::MultiServer::addressList( cfg.listeningAddresses("admin") , cfg.adminPort() ) ,
			cfg.allowRemoteClients() ,
			cfg.serverAddress() ,
			cfg.connectionTimeout() ,
			info_map ,
			config_map ,
			cfg.withTerminate() ) ) ;
}

void Main::Admin::notify( GSmtp::AdminServer & s , const std::string & p1 ,
	const std::string & p2 , const std::string & p3 )
{
	s.notify( p1 , p2 , p3 ) ;
}

void Main::Admin::report( const GSmtp::AdminServer & s )
{
	s.report() ;
}
/// \file admin.cpp
