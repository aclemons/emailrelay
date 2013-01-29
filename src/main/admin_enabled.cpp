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
//
// admin_enabled.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "admin.h"
#include "legal.h"
#include "gssl.h"

bool Main::Admin::enabled()
{
	return true ;
}

std::auto_ptr<GSmtp::AdminServer> Main::Admin::newServer( const Configuration & cfg , 
	GSmtp::MessageStore & store , const GSmtp::Client::Config & client_config , 
	const GAuth::Secrets & client_secrets , const std::string & version_number )
{
	GNet::Address local_address = 
		cfg.clientInterface().length() ? 
			GNet::Address(cfg.clientInterface(),0U) :
			GNet::Address(0U) ;

	G::StringMap extra_commands_map ;
	extra_commands_map.insert( std::make_pair(std::string("version"),version_number) ) ;
	extra_commands_map.insert( std::make_pair(std::string("warranty"),
		Legal::warranty(std::string(),std::string(1U,'\n'))) ) ;
	extra_commands_map.insert( std::make_pair(std::string("credit"),
		GSsl::Library::credit(std::string(),std::string(1U,'\n'),std::string())) ) ;
	extra_commands_map.insert( std::make_pair(std::string("copyright"),Legal::copyright()) ) ;

	std::auto_ptr<GSmtp::AdminServer> server ;
	server <<= new GSmtp::AdminServer( 
			store , 
			client_config ,
			client_secrets , 
			GNet::MultiServer::addressList( cfg.listeningInterfaces("admin") , cfg.adminPort() ) ,
			cfg.allowRemoteClients() , 
			local_address ,
			cfg.serverAddress() ,
			cfg.connectionTimeout() ,
			extra_commands_map ,
			cfg.withTerminate() ) ;

	return server ;
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
/// \file admin_enabled.cpp
