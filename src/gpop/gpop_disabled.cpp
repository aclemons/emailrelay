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
// gpop_disabled.cpp
//

#include "gdef.h"
#include "gpop.h"
#include "gpopserver.h"
#include "gpopsecrets.h"
#include "gpopstore.h"
#include "gexception.h"
#include "gstrings.h"

std::string GPop::Secrets::defaultPath()
{
	return std::string() ; // the empty string disables some other stuff
}

GPop::Secrets::Secrets( const std::string & )
{
}

GPop::Secrets::~Secrets()
{
}

bool GPop::Secrets::valid() const
{
	return false ;
}

std::string GPop::Secrets::source() const
{
	return std::string() ;
}

std::string GPop::Secrets::secret( const std::string & , const std::string & ) const
{
	return std::string() ;
}

bool GPop::Secrets::contains( const std::string & ) const
{
	return false ;
}

// ==

GPop::Store::Store( G::Path , bool , bool )
{
}

// ==

GPop::Server::Config::Config( bool , unsigned int , const G::Strings & )
{
}

// ==

G_EXCEPTION( NotImplemented , "cannot do pop: pop support not enabled at build time" ) ;

GPop::Server::Server( Store & store , const Secrets & secrets , Config ) :
	m_store(store) ,
	m_secrets(secrets)
{
	throw NotImplemented() ;
}

GPop::Server::~Server()
{
}

void GPop::Server::report() const
{
}

GNet::ServerPeer * GPop::Server::newPeer( GNet::Server::PeerInfo )
{
	return NULL ;
}


/// \file gpop_disabled.cpp
