//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gpopserver.cpp
//

#include "gdef.h"
#include "gpop.h"
#include "gpopserver.h"
#include "gmemory.h"
#include "glocal.h"
#include "glog.h"
#include "gdebug.h"
#include "gassert.h"
#include <string>

GPop::ServerPeer::ServerPeer( GNet::Server::PeerInfo peer_info , Server & server , Store & store , 
	const Secrets & secrets , std::auto_ptr<ServerProtocol::Text> ptext ,
	ServerProtocol::Config protocol_config ) :
		GNet::Sender( peer_info ) ,
		m_server( server ) ,
		m_buffer_in( crlf() ) ,
		m_ptext( ptext ) ,
		m_protocol( *this , store , secrets , *m_ptext.get() , peer_info.m_address , protocol_config )
{
	G_LOG_S( "GPop::ServerPeer: pop connection from " << peer_info.m_address.displayString() ) ;
	m_protocol.init() ;
}

std::string GPop::ServerPeer::crlf()
{
	return std::string("\015\012") ;
}

void GPop::ServerPeer::onDelete()
{
	G_LOG_S( "GPop::ServerPeer: pop connection closed: " << peerAddress().second.displayString() ) ;
}

void GPop::ServerPeer::onData( const char * p , size_t n )
{
	try
	{
		for( m_buffer_in.add(p,n) ; m_buffer_in.more() ; m_buffer_in.discard() )
		{
			processLine( m_buffer_in.current() ) ;
		}
	}
	catch( std::exception & e )
	{
		G_LOG( "GPop::onData: " << e.what() ) ;
		doDelete() ;
	}
}

void GPop::ServerPeer::processLine( const std::string & line )
{
	m_protocol.apply( line ) ;
}

bool GPop::ServerPeer::protocolSend( const std::string & line , size_t offset )
{
	return send( line , offset ) ; // GNet::Sender
}

void GPop::ServerPeer::onResume()
{
	m_protocol.resume() ; // calls back to protocolSend()
}

// ===

GPop::Server::Server( Store & store , const Secrets & secrets , Config config ) :
	m_allow_remote(config.allow_remote) ,
	m_store(store) ,
	m_secrets(secrets)
{
	GNet::Address address = 
		config.address.empty() ? 
			GNet::Address(config.port) : 
			GNet::Address(config.address,config.port) ;

	init( address ) ; // GNet::Server
}

void GPop::Server::report() const
{
	G_LOG_S( "GPop::Server: pop server on " << address().second.displayString() ) ;
	G_LOG_S( "GPop::Server: pop authentication secrets from \"" << m_secrets.path() << "\"" ) ;
}

GNet::ServerPeer * GPop::Server::newPeer( GNet::Server::PeerInfo peer_info )
{
	if( ! m_allow_remote && 
		!peer_info.m_address.sameHost(GNet::Local::canonicalAddress()) &&
		!peer_info.m_address.sameHost(GNet::Local::localhostAddress()) )
	{
		G_WARNING( "GPop::Server: configured to reject non-local connection: " 
			<< peer_info.m_address.displayString(false) << " is not one of " 
			<< GNet::Local::canonicalAddress().displayString(false) << ","
			<< GNet::Local::localhostAddress().displayString(false) ) ;
		return NULL ;
	}

	std::auto_ptr<ServerProtocol::Text> ptext( newProtocolText(peer_info.m_address) ) ;
	return new ServerPeer( peer_info , *this , m_store , m_secrets , 
		ptext , ServerProtocol::Config() ) ;
}

GPop::ServerProtocol::Text * GPop::Server::newProtocolText( GNet::Address peer_address ) const
{
	return new ServerProtocolText( peer_address ) ;
}

// ===

GPop::Server::Config::Config() :
	allow_remote(false) ,
	port(110)
{
}

GPop::Server::Config::Config( bool allow_remote_ , unsigned int port_ , const std::string & address_ ) :
		allow_remote(allow_remote_) ,
		port(port_) ,
		address(address_)
{
}

