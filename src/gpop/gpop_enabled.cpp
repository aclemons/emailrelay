//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gpop_enabled.cpp
///

#include "gdef.h"
#include "gpop.h"
#include "gsecrets.h"

bool GPop::enabled() noexcept
{
	return true ;
}

std::unique_ptr<GPop::Store> GPop::newStore( const G::Path & spool_dir , const Store::Config & config )
{
	return std::make_unique<Store>( spool_dir , config ) ;
}

std::unique_ptr<GAuth::SaslServerSecrets> GPop::newSecrets( const std::string & path )
{
	return GAuth::Secrets::newServerSecrets( path , "pop-server" ) ;
}

std::unique_ptr<GPop::Server> GPop::newServer( GNet::EventState es , Store & store ,
	const GAuth::SaslServerSecrets & secrets , const Server::Config & config )
{
	return std::make_unique<Server>( es , store , secrets , config ) ;
}

void GPop::report( const Server * server , const std::string & group )
{
	if( server )
		server->report( group ) ;
}
