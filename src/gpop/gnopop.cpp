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
// gnopop.cpp
//

#include "gdef.h"
#include "gpop.h"
#include "gpopserver.h"
#include "gpopsecrets.h"
#include "gpopstore.h"

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

std::string GPop::Secrets::secret(  const std::string & , const std::string & ) const
{
	return std::string() ;
}

// ==

GPop::Store::Store( G::Path , bool , bool )
{
}

// ==

GPop::Server::Config::Config( bool , unsigned int , const std::string & )
{
}

GPop::Server::Server( Store & store , const Secrets & secrets , Config ) :
	m_store(store) ,
	m_secrets(secrets)
{
}

void GPop::Server::report() const
{
}

GNet::ServerPeer * GPop::Server::newPeer( GNet::Server::PeerInfo )
{
	return NULL ;
}


